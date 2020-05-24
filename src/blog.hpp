#pragma once

#include <map>
#include <string>

namespace blog {
    using PostMap = std::map<std::string, std::string>;
    auto makePosts(std::string basePath) -> PostMap;
}
