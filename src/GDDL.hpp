#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <matjson.hpp>

#include <chrono>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gddl {
    inline constexpr char const* API_BASE = "https://gdladder.com/api";
    inline constexpr char const* SITE_BASE = "https://gdladder.com";

    /// Result of looking a level up on the ladder.
    struct RankInfo {
        /// false if the level has no GDDL rating (not on the ladder).
        bool found = false;
        /// GDDL rating (a float between 1 and 39).
        double rating = 0.0;
        /// Place on the ladder exactly as the site shows it: the "Position"
        /// field of the level page (`DifficultyIndex` in the API). Levels with
        /// the same rating share the place. 0 if the site did not send it.
        int rank = 0;
    };

    /// One row of the top levels list.
    struct Row {
        int id = 0;
        double rating = 0.0;
        std::string name;
        std::string author;
    };

    /// One page of the top levels list.
    struct Page {
        int page = 0;
        int limit = 0;
        int total = 0;
        std::vector<Row> rows;
    };

    /// Builds a request with a custom user agent and a timeout.
    geode::utils::web::WebRequest makeRequest();

    /// Reads the body of `GET /api/levels/{id}` (the same document the level
    /// page of the site is built from).
    RankInfo parseLevelInfo(matjson::Value const& json);

    std::string apiLevelUrl(int levelID);
    std::string siteLevelUrl(int levelID);

    /// Shortens `text` to `maxLength` characters, appending ".." when cut.
    std::string truncate(std::string const& text, std::size_t maxLength);

    /// In-memory cache shared by all UI elements.
    class Client {
    public:
        static Client& get();

        std::optional<RankInfo> getRank(int levelID);
        void storeRank(int levelID, RankInfo const& info);

        std::optional<Page> getPage(int page, int limit);
        void storePage(Page const& page);

        void clear();

    private:
        struct TimedRank {
            RankInfo info;
            std::chrono::steady_clock::time_point time;
        };
        struct TimedPage {
            Page page;
            std::chrono::steady_clock::time_point time;
        };

        std::unordered_map<int, TimedRank> m_ranks;
        std::map<std::pair<int, int>, TimedPage> m_pages;
    };
}
