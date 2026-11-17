#include "GDDL.hpp"

#include <fmt/format.h>

#include <utility>

using namespace geode::prelude;

namespace gddl {
    namespace {
        constexpr std::chrono::seconds RANK_TTL{5 * 60};
        constexpr std::chrono::seconds PAGE_TTL{2 * 60};

        bool isFresh(std::chrono::steady_clock::time_point const& time, std::chrono::seconds ttl) {
            return std::chrono::steady_clock::now() - time <= ttl;
        }
    }

    geode::utils::web::WebRequest makeRequest() {
        geode::utils::web::WebRequest request;
        request.header("User-Agent", "slesh.gddl-ladder/1.0 (Geode mod for Geometry Dash)");
        request.timeout(std::chrono::seconds(20));
        return request;
    }

    std::string apiLevelUrl(int levelID) {
        return fmt::format("{}/levels/{}", API_BASE, levelID);
    }

    RankInfo parseLevelInfo(matjson::Value const& json) {
        RankInfo info;
        // The API answers with an empty object for levels that are not rated.
        if (!json.contains("Rating")) {
            return info;
        }
        info.rating = json["Rating"].asDouble().unwrapOr(0.0);
        if (info.rating <= 0.0) {
            return info;
        }
        info.found = true;
        // Same value the "Position" field of the level page is rendered from.
        info.rank = static_cast<int>(json["DifficultyIndex"].asDouble().unwrapOr(0.0));
        return info;
    }

    std::string siteLevelUrl(int levelID) {
        return fmt::format("{}/level/{}", SITE_BASE, levelID);
    }

    std::string truncate(std::string const& text, std::size_t maxLength) {
        if (text.size() <= maxLength) {
            return text;
        }
        if (maxLength <= 2) {
            return text.substr(0, maxLength);
        }
        return text.substr(0, maxLength - 2) + "..";
    }

    Client& Client::get() {
        static Client instance;
        return instance;
    }

    std::optional<RankInfo> Client::getRank(int levelID) {
        auto it = m_ranks.find(levelID);
        if (it == m_ranks.end()) {
            return std::nullopt;
        }
        if (!isFresh(it->second.time, RANK_TTL)) {
            m_ranks.erase(it);
            return std::nullopt;
        }
        return it->second.info;
    }

    void Client::storeRank(int levelID, RankInfo const& info) {
        m_ranks[levelID] = TimedRank{info, std::chrono::steady_clock::now()};
    }

    std::optional<Page> Client::getPage(int page, int limit) {
        auto it = m_pages.find({page, limit});
        if (it == m_pages.end()) {
            return std::nullopt;
        }
        if (!isFresh(it->second.time, PAGE_TTL)) {
            m_pages.erase(it);
            return std::nullopt;
        }
        return it->second.page;
    }

    void Client::storePage(Page const& page) {
        m_pages[{page.page, page.limit}] = TimedPage{page, std::chrono::steady_clock::now()};
    }

    void Client::clear() {
        m_ranks.clear();
        m_pages.clear();
    }
}
