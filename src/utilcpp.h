#include <string>
#include <string_view>
#include <cstring>
#include <array>
#include <iterator>
#include <algorithm>
#include <vector>

using namespace std;

vector<string>
split_string(const string s, const char delim);

vector<string_view>
split_cppview(const char* src, const char* sep);

char*
join_cppview(vector<string_view> list, const char *sep);