#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/web.hpp>

#include "GDDL.hpp"

/// Paginated list of the hardest rated levels on gdladder.com.
class GDDLTopPopup : public geode::Popup {
public:
    static GDDLTopPopup* create();
    /// Opens the popup, unless an instance is already on screen.
    static void open();

protected:
    bool init();

private:
    cocos2d::CCMenu* m_rowsMenu = nullptr;
    cocos2d::CCNode* m_header = nullptr;
    cocos2d::CCLabelBMFont* m_info = nullptr;
    cocos2d::CCNode* m_infoRoot = nullptr;
    CCMenuItemSpriteExtra* m_prevBtn = nullptr;
    CCMenuItemSpriteExtra* m_nextBtn = nullptr;

    float m_rowWidth = 400.f;
    int m_perPage = 10;
    int m_page = 0;
    int m_total = -1;
    int m_rowCount = 0;
    bool m_loading = false;

    geode::async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_boundaryListener;

    void loadPage(int page, bool useCache);
    void onPageResponse(geode::utils::web::WebResponse response, int page);
    void finalizePage(gddl::Page data, int baseRank);
    void applyPage(gddl::Page const& data);
    void updateControls();
    void setInfoText(std::string const& text);
    void showError();

    void onPrev(cocos2d::CCObject* sender);
    void onNext(cocos2d::CCObject* sender);
    void onReload(cocos2d::CCObject* sender);
    void onOpenSite(cocos2d::CCObject* sender);
    void onRow(cocos2d::CCObject* sender);
};
