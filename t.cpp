#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>
int main() {
  // Get the current UTC time
  boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();


// Print the current UTC time
  std::cout << "Current UTC time is " << now << std::endl;
 

    std::string http_date = "Sun, 06 Nov 1994 08:49:37 GMT";

    // Convert the HTTP date string to a std::tm structure
    std::istringstream ss(http_date);
    std::locale loc(ss.getloc(), new boost::posix_time::time_input_facet("%a, %d %b %Y %H:%M:%S %Z"));
    ss.imbue(loc);
    std::tm tm = {};
    ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S %Z");

    // Convert the std::tm structure to a Boost Posix time
    boost::posix_time::ptime pe = boost::posix_time::ptime_from_tm(tm);

    // Print the Boost Posix time as a string
    std::cout << pe << std::endl;
    
    if (now < pe) {
      std::cout << "How is this possible";
      
    }
    else {
      std::cout << "more like it";
    }
    
    boost::posix_time::time_duration d (0, 0, 3600);
    
    now += d;
    std::cout << " UTC time after an hour is " << now << std::endl; 
    
    std::string CC = "max-age:232343, dfdf:fdsfc, fdsfcds";
    std::vector<std::string> v;
    boost::algorithm::split(v, CC, boost::algorithm::is_any_of(", "));
    
    for (int i = 0 ; i < v.size(); i ++){
      std::cout << v[i] << std::endl;
    }
    
    
    
    
    return 0;
}