/*
 * DAV FPS Bypass — SKELETON BUILD
 * Target: Geometry Dash Android 2.2081, Geode 4.5.0
 *
 * FASE INI SENGAJA DIBATASI supaya prioritas "BUILD SUCCESS" tercapai
 * dulu. Fitur yang di-drop sementara (akan ditambahkan bertahap SETELAH
 * skeleton ini terbukti build sukses di CI):
 *
 *   - PlayerObject::pushButton / releaseButton (perekaman input replay)
 *   - Sistem Replay (.davreplay) secara keseluruhan
 *   - Hook PlayLayer::init(...) — TIDAK dipakai di file ini karena
 *     signature-nya (jumlah & tipe parameter) berpotensi berbeda antar
 *     versi binding GD, dan saya belum memverifikasinya untuk 2.2081.
 *     Sebagai gantinya, label FPS counter dibuat secara "lazy" di dalam
 *     update() saat pertama kali dipanggil — ini menghindari kebutuhan
 *     meng-override init() sama sekali.
 *
 * Hook yang DIPAKAI di file ini, dan tingkat keyakinan saya:
 *
 *   1. PlayLayer::update(float dt)
 *      -> Signature paling stabil & paling sering dipakai di seluruh
 *         ekosistem mod Geode, termasuk contoh resmi dokumentasi Geode.
 *         Keyakinan: TINGGI.
 *
 *   2. PauseLayer::customSetup()
 *      -> Fungsi ini yang membangun tombol-tombol di pause menu (init()
 *         pada PauseLayer menangani boilerplate layer, customSetup()
 *         yang membangun UI). Pola menambah CCMenuItem ke m_buttonMenu
 *         di dalam customSetup() ini umum dipakai di komunitas mod
 *         Geode. Keyakinan: SEDANG-TINGGI, tapi BELUM saya cocokkan
 *         satu-satu dengan file binding PauseLayer di Geode SDK 4.5.0.
 *         KALAU compiler menolak dengan pesan semacam
 *         "customSetup does not override any base class member",
 *         buka file bindings PauseLayer di GEODE_SDK Anda
 *         (folder bindings/.../PauseLayer.bro atau dokumentasi
 *         https://docs.geode-sdk.org/bindings) dan sesuaikan nama/
 *         signature-nya — JANGAN menebak ulang tanpa mengecek.
 *
 *   3. CCDirector::sharedDirector()->setAnimationInterval(float)
 *      -> API cocos2d-x asli (bukan buatan Geode), sudah sangat lama
 *         stabil dan dipakai luas untuk unlock FPS. Keyakinan: TINGGI.
 *
 *   4. Mod::get()->setSavedValue<T>() / getSavedValue<T>()
 *      -> API resmi Geode untuk menyimpan nilai mod secara persisten.
 *         Keyakinan: TINGGI.
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
    inline void setPhysicsBypass(bool v) {
        Mod::get()->setSavedValue<bool>("physics-bypass", v);
    }

    inline int getTargetFps() {
        return Mod::get()->getSavedValue<int>("target-fps", 60);
    }
    inline void setTargetFps(int v) {
        Mod::get()->setSavedValue<int>("target-fps", v);
    }

    inline bool getFpsCounter() {
        return Mod::get()->getSavedValue<bool>("fps-counter", true);
    }
    inline void setFpsCounter(bool v) {
        Mod::get()->setSavedValue<bool>("fps-counter", v);
    }

} // namespace dav

// ============================================================
// 2. HOOK PlayLayer: hanya update(float dt).
//    Tidak meng-override init() sama sekali di fase ini.
// ============================================================

class $modify(DavPlayLayer, PlayLayer) {
public:
    struct Fields {
        CCLabelBMFont* fpsLabel = nullptr;
        float fpsAccumTime = 0.f;
        int fpsFrameCount = 0;
    };

    void update(float dt) {
        // --- Terapkan Target FPS (render only, tidak menyentuh data level) ---
        int fps = dav::getTargetFps();
        if (fps < 1) fps = 60;
        CCDirector::sharedDirector()->setAnimationInterval(1.f / static_cast<float>(fps));

        // --- Buat label FPS counter sekali saja, secara lazy ---
        if (dav::getFpsCounter() && !m_fields->fpsLabel) {
            m_fields->fpsLabel = CCLabelBMFont::create("FPS: --", "bigFont.fnt");
            m_fields->fpsLabel->setScale(0.4f);
            m_fields->fpsLabel->setAnchorPoint({0.f, 1.f});
            m_fields->fpsLabel->setPosition(
                {4.f, CCDirector::sharedDirector()->getWinSize().height - 4.f});
            m_fields->fpsLabel->setZOrder(1000);
            this->addChild(m_fields->fpsLabel);
        }
        if (m_fields->fpsLabel) {
            m_fields->fpsLabel->setVisible(dav::getFpsCounter());
        }

        // --- Physics Bypass: kunci dt gameplay ke 1/60 supaya fisika
        //     tidak berubah walau FPS render lebih tinggi. Ini murni
        //     memengaruhi delta waktu simulasi, tidak mengubah objek
        //     atau data level. ---
        float gameplayDt = dt;
        if (dav::getPhysicsBypass()) {
            gameplayDt = 1.f / 60.f;
        }

        PlayLayer::update(gameplayDt);

        // --- Hitung & tampilkan FPS render (pakai dt asli, bukan
        //     gameplayDt, supaya angkanya menggambarkan FPS sebenarnya) ---
        if (m_fields->fpsLabel && dav::getFpsCounter() && dt > 0.f) {
            m_fields->fpsFrameCount++;
            m_fields->fpsAccumTime += dt;
            if (m_fields->fpsAccumTime >= 0.5f) {
                int value = static_cast<int>(
                    std::round(m_fields->fpsFrameCount / m_fields->fpsAccumTime));
                m_fields->fpsLabel->setString(("FPS: " + std::to_string(value)).c_str());
                m_fields->fpsFrameCount = 0;
                m_fields->fpsAccumTime = 0.f;
            }
        }
    }
};

// ============================================================
// 3. POPUP MENU "DAV FPS Bypass"
// ============================================================

class DavMenuPopup : public geode::Popup<> {
protected:
    CCLabelBMFont* m_physicsLabel = nullptr;
    CCLabelBMFont* m_fpsLabel = nullptr;
    CCLabelBMFont* m_counterLabel = nullptr;

    void refreshLabels() {
        m_physicsLabel->setString(
            dav::getPhysicsBypass() ? "Physics Bypass: ON" : "Physics Bypass: OFF");
        m_fpsLabel->setString(
            ("Target FPS: " + std::to_string(dav::getTargetFps())).c_str());
        m_counterLabel->setString(
            dav::getFpsCounter() ? "FPS Counter: ON" : "FPS Counter: OFF");
    }

    bool setup() override {
        this->setTitle("DAV FPS Bypass");

        auto winSize = m_mainLayer->getContentSize();
        float centerX = winSize.width / 2.f;
        float topY = winSize.height - 55.f;

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

        m_physicsLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_physicsLabel->setScale(0.5f);
        auto physicsBtn = CCMenuItemSpriteExtra::create(
            m_physicsLabel, this, menu_selector(DavMenuPopup::onTogglePhysics));
        physicsBtn->setPosition({centerX, topY});
        menu->addChild(physicsBtn);

        m_fpsLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_fpsLabel->setScale(0.5f);
        auto fpsBtn = CCMenuItemSpriteExtra::create(
            m_fpsLabel, this, menu_selector(DavMenuPopup::onCycleFps));
        fpsBtn->setPosition({centerX, topY - 40.f});
        menu->addChild(fpsBtn);

        m_counterLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_counterLabel->setScale(0.5f);
        auto counterBtn = CCMenuItemSpriteExtra::create(
            m_counterLabel, this, menu_selector(DavMenuPopup::onToggleCounter));
        counterBtn->setPosition({centerX, topY - 80.f});
        menu->addChild(counterBtn);

        auto noteLabel = CCLabelBMFont::create(
            "Replay: segera hadir", "chatFont.fnt");
        noteLabel->setScale(0.5f);
        noteLabel->setPosition({centerX, topY - 120.f});
        m_mainLayer->addChild(noteLabel);

        refreshLabels();
        return true;
    }

    void onTogglePhysics(CCObject*) {
        dav::setPhysicsBypass(!dav::getPhysicsBypass());
        refreshLabels();
    }

    void onCycleFps(CCObject*) {
        int current = dav::getTargetFps();
        int idx = 0;
        for (int i = 0; i < dav::kFpsOptionCount; i++) {
            if (dav::kFpsOptions[i] == current) {
                idx = i;
                break;
            }
        }
        idx = (idx + 1) % dav::kFpsOptionCount;
        dav::setTargetFps(dav::kFpsOptions[idx]);
        refreshLabels();
    }

    void onToggleCounter(CCObject*) {
        dav::setFpsCounter(!dav::getFpsCounter());
        refreshLabels();
    }

public:
    static DavMenuPopup* create() {
        auto ret = new DavMenuPopup();
        if (ret && ret->initAnchored(260.f, 220.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ============================================================
// 4. HOOK PauseLayer: tambah tombol "DAV" ke pause menu
// ============================================================

class $modify(DavPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto label = CCLabelBMFont::create("DAV", "bigFont.fnt");
        label->setScale(0.6f);

        auto btn = CCMenuItemSpriteExtra::create(
            label, this, menu_selector(DavPauseLayer::onOpenDavMenu));
        btn->setID("dav-fps-bypass-button"_spr);

        if (m_buttonMenu) {
            m_buttonMenu->addChild(btn);
            m_buttonMenu->updateLayout();
        }
    }

    void onOpenDavMenu(CCObject*) {
        DavMenuPopup::create()->show();
    }
};
