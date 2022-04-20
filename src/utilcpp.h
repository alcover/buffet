#include <string>
#include <string_view>
#include <cstring>
#include <vector>

using namespace std;

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
