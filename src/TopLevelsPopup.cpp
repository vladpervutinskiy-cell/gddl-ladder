#include "TopLevelsPopup.hpp"

#include <Geode/utils/web.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

using namespace geode::prelude;

namespace {
    constexpr float MAX_POPUP_WIDTH = 440.f;
    constexpr float ROW_HEIGHT = 16.f;
    constexpr float ROWS_BOTTOM = 38.f;
    constexpr float SIDE_MARGIN = 20.f;
    constexpr int MIN_PER_PAGE = 5;
    constexpr int MAX_PER_PAGE = 12;

    cocos2d::ccColor3B const RANK_COLOR{255, 214, 92};
    cocos2d::ccColor3B const HEADER_COLOR{190, 190, 190};
    cocos2d::ccColor3B const MUTED_COLOR{200, 200, 200};

    /// Three columns: rank, "level - creator", rating. Coordinates are in the
    /// 0..rowWidth space of the node; CCMenuItemSprite anchors item contents
    /// at their bottom-left corner.
    cocos2d::CCNode* buildRow(
        float rowWidth,
        int levelID,
        std::string const& rankText,
        std::string const& mainText,
        std::string const& rightText,
        cocos2d::ccColor3B rankColor,
        cocos2d::ccColor3B mainColor,
        cocos2d::ccColor3B rightColor
    ) {
        auto node = cocos2d::CCNode::create();
        node->setContentSize({rowWidth, ROW_HEIGHT});

        auto addLabel = [&](std::string const& value, float x, float anchorX, cocos2d::ccColor3B color) {
            auto label = cocos2d::CCLabelBMFont::create(value.c_str(), "chatFont.fnt");
            label->setAnchorPoint({anchorX, 0.5f});
            label->setPosition({x, ROW_HEIGHT / 2.f});
            label->setScale(0.7f);
            label->setColor(color);
            node->addChild(label);
            return label;
        };

        auto rank = addLabel(rankText, 2.f, 0.f, rankColor);
        // The place arrives later than the list itself, so the label is found
        // back by this ID to be filled in.
        rank->setID(fmt::format("gddl-top-rank-{}", levelID));
        addLabel(mainText, 62.f, 0.f, mainColor);
        addLabel(rightText, rowWidth - 2.f, 1.f, rightColor);
        return node;
    }

    std::string rankLabel(gddl::RankInfo const& info) {
        if (!info.found) {
            return "#-";
        }
        if (info.rank <= 0) {
            return "#?";
        }
        return fmt::format("#{}", info.rank);
    }
}

GDDLTopPopup* GDDLTopPopup::create() {
    auto ret = new GDDLTopPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void GDDLTopPopup::open() {
    if (auto scene = cocos2d::CCDirector::get()->getRunningScene()) {
        if (scene->getChildByIDRecursive("gddl-top-popup"_spr)) {
            return;
        }
    }
    if (auto popup = GDDLTopPopup::create()) {
        popup->show();
    }
}

bool GDDLTopPopup::init() {
    m_perPage = static_cast<int>(Mod::get()->getSettingValue<std::int64_t>("levels-per-page"));
    m_perPage = std::clamp(m_perPage, MIN_PER_PAGE, MAX_PER_PAGE);

    auto winSize = cocos2d::CCDirector::get()->getWinSize();
    float popupWidth = std::min(MAX_POPUP_WIDTH, winSize.width - 16.f);
    m_rowWidth = popupWidth - SIDE_MARGIN * 2.f;

    float rowsHeight = m_perPage * ROW_HEIGHT;
    float height = 96.f + rowsHeight;

    if (!Popup::init(popupWidth, height)) {
        return false;
    }

    this->setID("gddl-top-popup"_spr);
    this->setTitle("GDDL - Top Levels");

    m_header = buildRow(m_rowWidth, 0, "#", "LEVEL - CREATOR", "GDDL", HEADER_COLOR, HEADER_COLOR, HEADER_COLOR);
    m_header->setID("gddl-top-header"_spr);
    m_header->setPosition({SIDE_MARGIN, ROWS_BOTTOM + rowsHeight + 2.f});
    m_mainLayer->addChild(m_header);

    m_rowsMenu = CCMenu::create();
    m_rowsMenu->setAnchorPoint({0.f, 0.f});
    m_rowsMenu->setContentSize({m_rowWidth, rowsHeight});
    m_rowsMenu->setPosition({SIDE_MARGIN, ROWS_BOTTOM});
    m_rowsMenu->setID("gddl-top-rows"_spr);
    m_mainLayer->addChild(m_rowsMenu);

    m_info = CCLabelBMFont::create("Loading...", "chatFont.fnt");
    m_info->setScale(0.65f);
    m_info->setColor(MUTED_COLOR);

    m_infoRoot = CCNode::create();
    m_infoRoot->addChild(m_info);
    this->setInfoText("Loading...");

    auto infoItem = CCMenuItemSpriteExtra::create(m_infoRoot, this, menu_selector(GDDLTopPopup::onReload));
    infoItem->setPosition({popupWidth / 2.f, 20.f});
    m_buttonMenu->addChild(infoItem);

    // Prev/Next are deliberately compact: they only hold a word each, so they
    // are laid out from their real width instead of fixed coordinates.
    constexpr float NAV_SCALE = 0.4f;
    constexpr float NAV_GAP = 6.f;
    constexpr float NAV_BOTTOM = 19.f;
    float navX = 14.f;

    auto prevSprite = ButtonSprite::create("PREV", "bigFont.fnt", "GJ_button_02.png", NAV_SCALE);
    m_prevBtn = CCMenuItemSpriteExtra::create(prevSprite, this, menu_selector(GDDLTopPopup::onPrev));
    float prevWidth = prevSprite->getContentSize().width * prevSprite->getScale();
    m_prevBtn->setPosition({navX + prevWidth / 2.f, NAV_BOTTOM});
    m_buttonMenu->addChild(m_prevBtn);
    navX += prevWidth + NAV_GAP;

    auto nextSprite = ButtonSprite::create("NEXT", "bigFont.fnt", "GJ_button_02.png", NAV_SCALE);
    m_nextBtn = CCMenuItemSpriteExtra::create(nextSprite, this, menu_selector(GDDLTopPopup::onNext));
    float nextWidth = nextSprite->getContentSize().width * nextSprite->getScale();
    m_nextBtn->setPosition({navX + nextWidth / 2.f, NAV_BOTTOM});
    m_buttonMenu->addChild(m_nextBtn);

    auto siteSprite = ButtonSprite::create("OPEN GDDL", "bigFont.fnt", "GJ_button_01.png", 0.5f);
    auto siteBtn = CCMenuItemSpriteExtra::create(siteSprite, this, menu_selector(GDDLTopPopup::onOpenSite));
    siteBtn->setPosition({popupWidth - 62.f, 20.f});
    m_buttonMenu->addChild(siteBtn);

    this->loadPage(0, true);
    return true;
}

void GDDLTopPopup::loadPage(int page, bool useCache) {
    if (page < 0) {
        page = 0;
    }

    if (useCache) {
        if (auto cached = gddl::Client::get().getPage(page, m_perPage)) {
            this->applyPage(*cached);
            return;
        }
    }

    m_page = page;
    m_loading = true;
    this->updateControls();

    auto request = gddl::makeRequest();
    request.param("excludeUnrated", "true");
    request.param("sort", "rating");
    request.param("sortDirection", "desc");
    request.param("limit", std::to_string(m_perPage));
    request.param("page", std::to_string(page));

    m_listener.spawn(
        request.get(fmt::format("{}/levels", gddl::API_BASE), Mod::get()),
        [this, page](web::WebResponse response) {
            this->onPageResponse(std::move(response), page);
        }
    );
}

void GDDLTopPopup::onPageResponse(web::WebResponse response, int page) {
    if (page != m_page) {
        return;
    }

    auto jsonResult = response.json();
    if (!response.ok() || !jsonResult.isOk()) {
        this->showError();
        return;
    }

    auto json = jsonResult.unwrap();
    if (!json.isObject()) {
        this->showError();
        return;
    }

    auto rowsResult = json["data"].asArray();
    if (rowsResult.isErr()) {
        this->showError();
        return;
    }

    gddl::Page data;
    data.page = page;
    data.limit = m_perPage;
    data.total = static_cast<int>(json["total"].asInt().unwrapOr(0));

    for (auto const& entry : rowsResult.unwrap()) {
        gddl::Row row;
        row.id = static_cast<int>(entry["id"].asInt().unwrapOr(0));
        row.rating = entry["rating"].asDouble().unwrapOr(0.0);
        row.name = entry["name"].asString().unwrapOr(std::string("?"));
        row.author = entry["publisherName"].asString().unwrapOr(std::string("?"));
        if (row.id <= 0) {
            continue;
        }
        data.rows.push_back(std::move(row));
    }

    // Every row gets the place from the level document, so no place has to be
    // derived from the ratings (the site counts places over a wider set of
    // levels than the public list returns).
    this->finalizePage(std::move(data));
}

void GDDLTopPopup::finalizePage(gddl::Page data) {
    data.limit = m_perPage;

    gddl::Client::get().storePage(data);
    this->applyPage(data);
}

void GDDLTopPopup::applyPage(gddl::Page const& data) {
    m_loading = false;
    m_page = data.page;
    m_rowCount = static_cast<int>(data.rows.size());
    if (data.total > 0) {
        m_total = data.total;
    }

    m_rowsMenu->removeAllChildren();
    m_rankListeners.clear();

    float rowsHeight = m_perPage * ROW_HEIGHT;
    std::vector<int> missing;
    for (int i = 0; i < static_cast<int>(data.rows.size()); ++i) {
        auto const& row = data.rows[i];
        auto color = gddl::ratingColor(row.rating);
        auto cached = gddl::Client::get().getRank(row.id);
        std::string rankText = "#..";
        if (cached) {
            rankText = rankLabel(*cached);
        } else {
            missing.push_back(row.id);
        }

        auto content = buildRow(
            m_rowWidth,
            row.id,
            rankText,
            fmt::format("{}  -  {}", gddl::truncate(row.name, 30), gddl::truncate(row.author, 16)),
            fmt::format("{:.2f}", row.rating),
            RANK_COLOR,
            color,
            color
        );

        auto item = CCMenuItemSpriteExtra::create(content, this, menu_selector(GDDLTopPopup::onRow));
        item->setTag(row.id);
        item->setPosition({m_rowWidth / 2.f, rowsHeight - ROW_HEIGHT / 2.f - i * ROW_HEIGHT});
        m_rowsMenu->addChild(item);
    }

    this->updateControls();
    this->requestRanks(std::move(missing));
}

void GDDLTopPopup::requestRanks(std::vector<int> levelIDs) {
    for (int levelID : levelIDs) {
        auto request = gddl::makeRequest();
        m_rankListeners.emplace_back();
        m_rankListeners.back().spawn(
            request.get(gddl::apiLevelUrl(levelID), Mod::get()),
            [this, levelID](web::WebResponse response) {
                this->onRankResponse(std::move(response), levelID);
            }
        );
    }
}

void GDDLTopPopup::onRankResponse(web::WebResponse response, int levelID) {
    auto jsonResult = response.json();
    if (!response.ok() || !jsonResult.isOk()) {
        this->setRowRank(levelID, "#?");
        return;
    }

    auto info = gddl::parseLevelInfo(jsonResult.unwrap());
    gddl::Client::get().storeRank(levelID, info);
    // The popup may have been closed or the page may have changed meanwhile;
    // setRowRank ignores unknown IDs in that case.
    this->setRowRank(levelID, rankLabel(info));
}

void GDDLTopPopup::setRowRank(int levelID, std::string const& text) {
    if (!m_rowsMenu) {
        return;
    }
    auto label = typeinfo_cast<cocos2d::CCLabelBMFont*>(
        m_rowsMenu->getChildByIDRecursive(fmt::format("gddl-top-rank-{}", levelID))
    );
    if (label) {
        label->setString(text.c_str());
    }
}

void GDDLTopPopup::setInfoText(std::string const& text) {
    if (!m_info || !m_infoRoot) {
        return;
    }
    m_info->setString(text.c_str());
    auto size = m_info->getScaledContentSize();
    m_infoRoot->setContentSize(size);
    m_info->setPosition({size.width / 2.f, size.height / 2.f});
    if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(m_infoRoot->getParent())) {
        item->setContentSize(size);
    }
}

void GDDLTopPopup::updateControls() {
    int pages = m_total > 0 ? (m_total + m_perPage - 1) / m_perPage : 0;

    if (m_loading) {
        this->setInfoText("Loading...");
    } else if (m_rowCount == 0) {
        this->setInfoText("No levels on this page");
    } else {
        this->setInfoText(
            fmt::format("{} levels | page {}/{}", m_total, m_page + 1, std::max(pages, 1))
        );
    }

    m_prevBtn->setEnabled(!m_loading && m_page > 0);
    m_nextBtn->setEnabled(!m_loading && m_rowCount > 0 && m_page + 1 < pages);
}

void GDDLTopPopup::showError() {
    m_loading = false;
    m_rowCount = 0;
    m_rowsMenu->removeAllChildren();
    this->setInfoText("Failed to load - press here to retry");
    m_prevBtn->setEnabled(m_page > 0);
    m_nextBtn->setEnabled(false);
}

void GDDLTopPopup::onPrev(CCObject*) {
    if (m_page > 0) {
        this->loadPage(m_page - 1, true);
    }
}

void GDDLTopPopup::onNext(CCObject*) {
    this->loadPage(m_page + 1, true);
}

void GDDLTopPopup::onReload(CCObject*) {
    this->loadPage(m_page, false);
}

void GDDLTopPopup::onOpenSite(CCObject*) {
    std::string url = gddl::SITE_BASE;
    geode::utils::web::openLinkInBrowser(url);
}

void GDDLTopPopup::onRow(CCObject* sender) {
    auto node = typeinfo_cast<CCNode*>(sender);
    if (!node) {
        return;
    }
    std::string url = gddl::siteLevelUrl(node->getTag());
    geode::utils::web::openLinkInBrowser(url);
}
