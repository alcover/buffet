#include <string>
#include <string_view>
#include <cstring>
#include <vector>
#include <iostream>

using namespace std;

// std::ostream& bold_on(std::ostream& os) {return os << "\e[1m";}
// std::ostream& bold_off(std::ostream& os) {return os << "\e[0m";}

vector<string>
split_string(const string s, const char delim);

vector<string_view>
split_cppview(const char* src, const char* sep);

// alco
char*
_join_cppview(vector<string_view> list, const char *sep);

// Justas Masiulis
char* 
join_cppview(const vector<string_view>& parts, string_view sep);
