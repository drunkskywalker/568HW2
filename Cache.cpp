#include "Cache.hpp"

// add ifstream reference, lock pointer
using namespace std;
using namespace boost;
using namespace boost::beast::http;
using namespace asio::ip;
using namespace boost::posix_time;

Cache::Cache(size_t capacity,
             pthread_rwlock_t * cache_rwlock,
             pthread_mutex_t * log_lock,
             ofstream & lFile) :
    capacity(capacity), cache_rwlock(cache_rwlock), log_lock(log_lock), lFile(lFile) {
}

bool Cache::try_add(request<dynamic_body> & req, response<dynamic_body> & res) {
  pthread_rwlock_wrlock(cache_rwlock);
  string url = get_url(req);
  if (request_list.size() < capacity) {
    response_map[url] = res;
    request_list.push_back(url);
    pthread_rwlock_unlock(cache_rwlock);
    return true;
  }
  else {
    string old = request_list.front();
    response_map.erase(old);
    request_list.erase(request_list.begin());
    response_map[url] = res;
    request_list.push_back(url);
    pthread_rwlock_unlock(cache_rwlock);
    return true;
  }
  pthread_rwlock_unlock(cache_rwlock);
  return false;
}

string Cache::get_url(request<dynamic_body> & req) {
  return string(req.at("HOST")) + string(req.target());
}
int Cache::get_index(string url) {
  pthread_rwlock_rdlock(cache_rwlock);
  for (size_t i = 0; i < request_list.size(); i++) {
    if (request_list[i] == url) {
      pthread_rwlock_unlock(cache_rwlock);
      return i;
    }
  }
  pthread_rwlock_unlock(cache_rwlock);
  return -1;
}
//checks if response has expired
string Cache::check_time(response<dynamic_body> & res) {
  ptime now = second_clock::universal_time();

  if (res.find("Cache-Control") != res.end() && res.find("Date") != res.end()) {
    istringstream ss(string(res.at("Date")));
    locale loc(ss.getloc(), new posix_time::time_input_facet("%a, %d %b %Y %H:%M:%S %Z"));
    ss.imbue(loc);
    tm tm = {};
    ss >> get_time(&tm, "%a, %d %b %Y %H:%M:%S %Z");
    posix_time::ptime res_date = posix_time::ptime_from_tm(tm);
    string CC = string(res.at("Cache-Control"));
    vector<string> vars;
    algorithm::split(vars, CC, algorithm::is_any_of(", "));
    int ma = -2;
    for (size_t i = 0; i < vars.size(); i++) {
      if (vars[i].substr(0, 8) == "max-age=") {
        int ma = stoi(vars[i].substr(8));
        if (ma == -1) {
          return "-1";
        }
      }
    }
    if (ma != -2) {
      posix_time::time_duration d(0, 0, ma);

      posix_time::ptime ex = res_date + d;
      if (now >= ex) {
        stringstream ss_re;
        ss_re << ex;
        return ss_re.str();
      }
    }
  }

  if (res.find("Expires") != res.end()) {
    if (string(res.at("Expires")) == "-1") {
      return "0";
    }
    istringstream ss(string(res.at("Expires")));
    locale loc(ss.getloc(), new posix_time::time_input_facet("%a, %d %b %Y %H:%M:%S %Z"));
    ss.imbue(loc);
    std::tm tm = {};
    ss >> get_time(&tm, "%a, %d %b %Y %H:%M:%S %Z");
    ptime exp = ptime_from_tm(tm);
    if (now >= exp) {
      stringstream ss_re;
      ss_re << exp;
      return ss_re.str();
    }
  }

  return "0";
}
//revalidates with server
bool Cache::check_with_server(int id,
                              request<dynamic_body> & req,
                              response<dynamic_body> & res,
                              tcp::socket * user_sock,
                              tcp::socket * server_sock) {
  // cout << res;
  if (res.find(field::last_modified) != res.end()) {
    // cout << "has lm";
    req.set(field::if_modified_since, string(res.at("Last-Modified")));
  }
  if (res.find(field::etag) != res.end()) {
    // cout << "has etag";
    req.set(beast::http::field::if_none_match, string(res.at("ETag")));
  }

  cout << req;
  string ver;
  if (req.version() == 10) {
    ver = "HTTP/1.0";
  }
  else {
    ver = "HTTP/1.1";
  }
  pthread_mutex_lock(log_lock);
  posix_time::ptime recv_time = posix_time::second_clock::universal_time();
  lFile << id << ": re-validation with server" << endl;
  lFile << id << ": " << req.method() << " " << req.target() << " " << ver << " from "
        << user_sock->remote_endpoint().address() << " @ " << recv_time << endl;
  pthread_mutex_unlock(log_lock);
  write(*server_sock, req);

  response<dynamic_body> res_new;
  beast::flat_buffer response_buffer;
  read(*server_sock, response_buffer, res);

  if (res.version() == 10) {
    ver = "HTTP/1.0";
  }
  else {
    ver = "HTTP/1.1";
  }
  pthread_mutex_lock(log_lock);
  lFile << id << ": Received \"" << ver << " " << res_new.result_int() << " "
        << res_new.result() << "\" from " << req.at("HOST") << endl;
  pthread_mutex_unlock(log_lock);
  if (res_new.result_int() == 304) {
    return true;
  }
  else if (res_new.result_int() == 200) {
    try_add(req, res_new);
    cout << res_new;
    return false;
  }
  return false;
}

//check if response is allowed to store
// new request, new response
bool Cache::check_store(int id,
                        request<dynamic_body> & req,
                        response<dynamic_body> & res) {
  if (req.method_string() != "GET") {
    return false;
  }
  if (res.result_int() != 200) {
    pthread_mutex_lock(log_lock);
    lFile << id << ": not cachable because not 200 OK" << endl;
    pthread_mutex_unlock(log_lock);
    return false;
  }

  if (res.find("Cache-Control") != res.end()) {
    string CC = string(res.at("Cache-Control"));
    vector<string> vars;
    algorithm::split(vars, CC, algorithm::is_any_of(", "));
    for (size_t i = 0; i < vars.size(); i++) {
      if (vars[i] == "no-store") {
        pthread_mutex_lock(log_lock);
        lFile << id << ": not cachable because Cache-Control no-store" << endl;
        pthread_mutex_unlock(log_lock);
        return false;
      }
      else if (vars[i] == "no-cache") {
        pthread_mutex_lock(log_lock);
        lFile << id << ": cached, but requires re-validation" << endl;
        pthread_mutex_unlock(log_lock);
        return true;
      }
    }
  }
  string exptime = check_time(res);
  if (exptime == "-1") {
    pthread_mutex_lock(log_lock);
    lFile << id << ": not cachable because Cache-Control max-age=-1" << endl;
    pthread_mutex_unlock(log_lock);
    return false;
  }
  else if (exptime != "0") {
    pthread_mutex_lock(log_lock);
    lFile << id << ": cached, expires at " << exptime << endl;
    pthread_mutex_unlock(log_lock);
    return true;
  }

  pthread_mutex_lock(log_lock);
  lFile << id << ": cached, expires at -1" << endl;
  pthread_mutex_unlock(log_lock);

  return true;
}

bool Cache::check_nocache(response<dynamic_body> & res) {
  if (res.find("Cache-Control") != res.end()) {
    string CC = string(res.at("Cache-Control"));
    vector<string> vars;
    algorithm::split(vars, CC, algorithm::is_any_of(", "));
    for (size_t i = 0; i < vars.size(); i++) {
      if (vars[i] == "no-cache") {
        return true;
      }
    }
  }
  return false;
}

// TODO: check if this cache is ready to be reused
int Cache::check_read(int id,
                      request<dynamic_body> & req,
                      tcp::socket * user_sock,
                      tcp::socket * server_sock) {
  string url = get_url(req);
  if (get_index(url) == -1) {
    // not cached

    pthread_mutex_lock(log_lock);
    lFile << id << ": not in cache\n";
    pthread_mutex_unlock(log_lock);
    return -1;
  }

  response<dynamic_body> res = response_map[url];

  bool nocache = check_nocache(res);
  string exp = check_time(res);

  if (!nocache && exp == "0") {
    pthread_mutex_lock(log_lock);
    lFile << id << ": in cache, valid\n";
    pthread_mutex_unlock(log_lock);
    pthread_rwlock_unlock(cache_rwlock);
    return 0;
  }

  if (check_time(res) != "0") {
    pthread_mutex_lock(log_lock);
    lFile << id << ": in cache, but expired at " << exp << endl;
    pthread_mutex_unlock(log_lock);
  }

  if (nocache) {
    pthread_mutex_lock(log_lock);
    lFile << id << ": in cache, requires revalidation\n";
    pthread_mutex_unlock(log_lock);
  }
  if (!check_with_server(id, req, res, user_sock, server_sock)) {
    // server expired
    return -3;
  }

  // allowed
  return 0;
}
// only works if check_read() return 0
response<dynamic_body> Cache::get_cached_response(request<dynamic_body> & req) {
  string url = get_url(req);
  int index = get_index(url);
  pthread_rwlock_wrlock(cache_rwlock);
  request_list.erase(request_list.begin() + index);
  request_list.push_back(url);
  response<dynamic_body> res = response_map[url];
  pthread_rwlock_unlock(cache_rwlock);
  return res;
}
