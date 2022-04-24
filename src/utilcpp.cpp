#include "utilcpp.h"

vector<string>
split_string(const string s, const char delim)
{
    vector<string> elems;
    size_t beg{};
    size_t end{};

    do {
        end = s.find_first_of(delim, beg);
        elems.emplace_back(s.substr(beg, end - beg));
        beg = end + 1;
    } while (end != string::npos);

    return elems;
}


// slow.. ?
vector<string_view>
split_cppview(const char* src, const char* sep)
{
    const string_view view(src);
    const size_t seplen = strlen(sep);
    vector<string_view> parts;
    size_t beg{};
    size_t end{};

    do {
        end = view.find_first_of(sep, beg);
        parts.emplace_back(view.substr(beg, end-beg));
        beg = end + seplen;
    } while (end != string::npos);

    return parts;
}

// alco
char*
_join_cppview(vector<string_view> parts, const char *sep)
{
	const int cnt = parts.size();
    const size_t seplen = strlen(sep);
	size_t *lengths = (size_t*)malloc(cnt*sizeof(*lengths));
    size_t totlen = 0;

    for (int i=0; i<cnt; ++i) {
        size_t len = parts[i].size();
        totlen += len;
        lengths[i] = len;
    }
    totlen += (cnt-1)*seplen;

    char *ret = (char*)malloc(totlen+1);
    ret[totlen] = 0; 
    char *cur = ret;

    for (int i=0; i < cnt; ++i) {
        size_t eltlen = lengths[i];
        memcpy(cur, parts[i].data(), eltlen);
        cur += eltlen;
        if (i<cnt-1) {
            memcpy(cur, sep, seplen);
            cur += seplen;
        }
    }    

    free(lengths);

    return ret;
}


// Justas Masiulis
char* 
join_cppview(const std::vector<std::string_view>& parts, std::string_view sep)
{
    if(parts.empty()) return nullptr;

    size_t total = sep.size() * (parts.size() - 1);
    for (auto&& part : parts) total += part.size();

    auto ret = (char*)malloc(total+1); //+1:alco
    if (!ret) return nullptr;
    ret[total] = 0; //alco

    auto ptr = ret;
    for(auto&& part : parts) {
        memcpy(ptr, part.data(), part.size());
        ptr += part.size();

        if(&part != &parts.back()) {
            memcpy(ptr, sep.data(), sep.size());
            ptr += sep.size();
        }
    }

    return ret;
}