#include "utilcpp.h"

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