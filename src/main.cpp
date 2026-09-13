/*
 * DAV FPS Bypass — SKELETON BUILD
 * Target: Geometry Dash Android 2.2081, Geode 5.8.2
 *
 * Fase ini difokuskan untuk BUILD SUCCESS terlebih dahulu.
 * Replay dan input recording akan ditambahkan setelah skeleton
 * berhasil dibangun di GitHub Actions.
 */

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cmath>
#include <string>

using namespace geode::prelude;

// ============================================================
// 1. STATE / PENGATURAN TERSIMPAN
// ============================================================

namespace dav {

    constexpr int kFpsOptions[] = {60, 120, 144, 240};
    constexpr int kFpsOptionCount = 4;

    inline bool getPhysicsBypass() {
        return Mod::get()->getSavedValue<bool>("physics-bypass", false);
    }

    inline void setPhysicsBypass(bool value) {
        Mod::get()->setSavedValue<bool>("physics-bypass", value);
    }

    inline int getTargetFps() {
        return Mod::get()->getSavedValue<int>("target-fps", 60);
    }

    inline void setTargetFps(int value) {
        Mod::get()->setSavedValue<int>("target-fps", value);
    }

    inline bool getFpsCounter() {
        return Mod::get()->getSavedValue<bool>("fps-counter", true);
    }

    inline void setFpsCounter(bool value) {
        Mod::get()->setSavedValue<bool>("fps-counter", value);
    }

} // namespace dav


// ============================================================
// 2. HOOK PlayLayer
// ============================================================

class $modify(DavPlayLayer, PlayLayer) {
public:

    struct Fields {
        CCLabelBMFont* fpsLabel = nullptr;
        float fpsAccumTime = 0.f;
        int fpsFrameCount = 0;
    };

    void update(float dt) {

        // ----------------------------------------------------
        // Target FPS
        // ----------------------------------------------------

        int fps = dav::getTargetFps();

        if (fps < 1) {
            fps = 60;
        }

        CCDirector::sharedDirector()->setAnimationInterval(
            1.f / static_cast<float>(fps)
        );


        // ----------------------------------------------------
        // FPS Counter
        // ----------------------------------------------------

        if (dav::getFpsCounter() && !m_fields->fpsLabel) {

            m_fields->fpsLabel =
                CCLabelBMFont::create("FPS: --", "bigFont.fnt");

            m_fields->fpsLabel->setScale(0.4f);

            m_fields->fpsLabel->setAnchorPoint({0.f, 1.f});

            m_fields->fpsLabel->setPosition({
                4.f,
                CCDirector::sharedDirector()
                    ->getWinSize()
                    .height - 4.f
            });

            m_fields->fpsLabel->setZOrder(1000);

            this->addChild(m_fields->fpsLabel);
        }

        if (m_fields->fpsLabel) {
            m_fields->fpsLabel->setVisible(
                dav::getFpsCounter()
            );
        }


        // ----------------------------------------------------
        // Physics Bypass
        // ----------------------------------------------------

        float gameplayDt = dt;

        if (dav::getPhysicsBypass()) {
            gameplayDt = 1.f / 60.f;
        }

        PlayLayer::update(gameplayDt);


        // ----------------------------------------------------
        // Hitung FPS sebenarnya
        // ----------------------------------------------------

        if (
            m_fields->fpsLabel &&
            dav::getFpsCounter() &&
            dt > 0.f
        ) {

            m_fields->fpsFrameCount++;
            m_fields->fpsAccumTime += dt;

            if (m_fields->fpsAccumTime >= 0.5f) {

                int value = static_cast<int>(
                    std::round(
                        m_fields->fpsFrameCount /
                        m_fields->fpsAccumTime
                    )
                );

                std::string text =
                    "FPS: " + std::to_string(value);

                m_fields->fpsLabel->setString(text.c_str());

                m_fields->fpsFrameCount = 0;
                m_fields->fpsAccumTime = 0.f;
            }
        }
    }
};


// ============================================================
// 3. DAV FPS BYPASS MENU
// ============================================================

class DavMenuPopup : public geode::Popup<> {

protected:

    CCLabelBMFont* m_physicsLabel = nullptr;
    CCLabelBMFont* m_fpsLabel = nullptr;
    CCLabelBMFont* m_counterLabel = nullptr;


    void refreshLabels() {

        m_physicsLabel->setString(
            dav::getPhysicsBypass()
                ? "Physics Bypass: ON"
                : "Physics Bypass: OFF"
        );

        std::string fpsText =
            "Target FPS: " +
            std::to_string(dav::getTargetFps());

        m_fpsLabel->setString(fpsText.c_str());

        m_counterLabel->setString(
            dav::getFpsCounter()
                ? "FPS Counter: ON"
                : "FPS Counter: OFF"
        );
    }


    bool setup() override {

        this->setTitle("DAV FPS Bypass");

        auto winSize = m_mainLayer->getContentSize();

        float centerX = winSize.width / 2.f;
        float topY = winSize.height - 55.f;


        // ----------------------------------------------------
        // Menu
        // ----------------------------------------------------

        auto menu = CCMenu::create();

        menu->setPosition({0.f, 0.f});

        m_mainLayer->addChild(menu);


        // ----------------------------------------------------
        // Physics Bypass
        // ----------------------------------------------------

        m_physicsLabel =
            CCLabelBMFont::create("", "bigFont.fnt");

        m_physicsLabel->setScale(0.5f);

        auto physicsBtn =
            CCMenuItemSpriteExtra::create(
                m_physicsLabel,
                this,
                menu_selector(
                    DavMenuPopup::onTogglePhysics
                )
            );

        physicsBtn->setPosition({
            centerX,
            topY
        });

        menu->addChild(physicsBtn);


        // ----------------------------------------------------
        // Target FPS
        // ----------------------------------------------------

        m_fpsLabel =
            CCLabelBMFont::create("", "bigFont.fnt");

        m_fpsLabel->setScale(0.5f);

        auto fpsBtn =
            CCMenuItemSpriteExtra::create(
                m_fpsLabel,
                this,
                menu_selector(
                    DavMenuPopup::onCycleFps
                )
            );

        fpsBtn->setPosition({
            centerX,
            topY - 40.f
        });

        menu->addChild(fpsBtn);


        // ----------------------------------------------------
        // FPS Counter
        // ----------------------------------------------------

        m_counterLabel =
            CCLabelBMFont::create("", "bigFont.fnt");

        m_counterLabel->setScale(0.5f);

        auto counterBtn =
            CCMenuItemSpriteExtra::create(
                m_counterLabel,
                this,
                menu_selector(
                    DavMenuPopup::onToggleCounter
                )
            );

        counterBtn->setPosition({
            centerX,
            topY - 80.f
        });

        menu->addChild(counterBtn);


        // ----------------------------------------------------
        // Replay placeholder
        // ----------------------------------------------------

        auto noteLabel =
            CCLabelBMFont::create(
                "Replay: segera hadir",
                "chatFont.fnt"
            );

        noteLabel->setScale(0.5f);

        noteLabel->setPosition({
            centerX,
            topY - 120.f
        });

        m_mainLayer->addChild(noteLabel);


        refreshLabels();

        return true;
    }


    // --------------------------------------------------------
    // Toggle Physics
    // --------------------------------------------------------

    void onTogglePhysics(CCObject*) {

        dav::setPhysicsBypass(
            !dav::getPhysicsBypass()
        );

        refreshLabels();
    }


    // --------------------------------------------------------
    // Cycle FPS
    // --------------------------------------------------------

    void onCycleFps(CCObject*) {

        int current = dav::getTargetFps();

        int idx = 0;

        for (
            int i = 0;
            i < dav::kFpsOptionCount;
            i++
        ) {

            if (dav::kFpsOptions[i] == current) {
                idx = i;
                break;
            }
        }

        idx =
            (idx + 1) %
            dav::kFpsOptionCount;

        dav::setTargetFps(
            dav::kFpsOptions[idx]
        );

        refreshLabels();
    }


    // --------------------------------------------------------
    // Toggle FPS Counter
    // --------------------------------------------------------

    void onToggleCounter(CCObject*) {

        dav::setFpsCounter(
            !dav::getFpsCounter()
        );

        refreshLabels();
    }


public:

    static DavMenuPopup* create() {

        auto ret = new DavMenuPopup();

        if (
            ret &&
            ret->initAnchored(260.f, 220.f)
        ) {

            ret->autorelease();

            return ret;
        }

        CC_SAFE_DELETE(ret);

        return nullptr;
    }
};


// ============================================================
// 4. HOOK PauseLayer
// ============================================================

class $modify(DavPauseLayer, PauseLayer) {

    void customSetup() {

        PauseLayer::customSetup();


        // ----------------------------------------------------
        // DAV Button
        // ----------------------------------------------------

        auto label =
            CCLabelBMFont::create(
                "DAV",
                "bigFont.fnt"
            );

        label->setScale(0.6f);


        auto btn =
            CCMenuItemSpriteExtra::create(
                label,
                this,
                menu_selector(
                    DavPauseLayer::onOpenDavMenu
                )
            );

        btn->setID(
            "dav-fps-bypass-button"_spr
        );


        if (m_buttonMenu) {

            m_buttonMenu->addChild(btn);

            m_buttonMenu->updateLayout();
        }
    }


    void onOpenDavMenu(CCObject*) {

        auto popup =
            DavMenuPopup::create();

        if (popup) {
            popup->show();
        }
    }
};
