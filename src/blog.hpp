#pragma once

#include <unordered_map>
#include <string>

namespace blog {
    using PostMap = std::unordered_map<std::string, std::string>;
    struct PostData {
        PostMap posts;
        std::string newestPostName;
        std::string rss;
    };
    auto makePosts(std::string basePath) -> PostData;
}
