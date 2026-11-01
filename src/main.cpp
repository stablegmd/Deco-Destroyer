
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

static constexpr int DECO_DESTROYER_TAG = 82941;

static bool isTeleportPortal(int id) {
    return id == 749 || id == 2064;
}

static bool isVisualOnlyTrigger(int id) {
    switch (id) {
        case 29:
        case 30:
        case 32:
        case 33:
        case 104:
        case 105:
        case 221:
        case 717:
        case 718:
        case 743:
        case 744:
        case 899:
        case 900:
        case 915:
        case 1006:
        case 1007:
        case 1520:
        case 2903:
        case 2904:
        case 2905:
        case 2907:
        case 2909:
        case 2910:
        case 2911:
        case 2912:
        case 2913:
        case 2914:
        case 2915:
        case 2916:
        case 2917:
        case 2919:
        case 2920:
        case 2921:
        case 2922:
        case 2923:
        case 2924:
            return true;

        default:
            return false;
    }
}

static bool isNoTouchSolidOrHazard(GameObject* object) {
    if (!object)
        return false;

    auto type = object->getType();

    if (type == GameObjectType::Solid ||
        type == GameObjectType::Hazard) {
        return true;
    }

    if (type == GameObjectType::Slope && object->m_slopeIsHazard) {
        return true;
    }

    return false;
}

class DecoDestroyerAlert : public FLAlertLayerProtocol {
public:
    EditorUI* m_editor;

    explicit DecoDestroyerAlert(EditorUI* editor)
        : m_editor(editor) {}

    void FLAlert_Clicked(FLAlertLayer*, bool confirmed) override {
        if (confirmed && m_editor && m_editor->m_editorLayer) {
            auto editorLayer = m_editor->m_editorLayer;
            auto objects = editorLayer->getAllObjects();

            bool staticColour =
                Mod::get()->getSettingValue<bool>("static-obj-colour");

            if (staticColour) {
                auto chosenColour =
                    Mod::get()->getSettingValue<ccColor3B>("object-colour");

                auto levelSettings = editorLayer->m_levelSettings;
                auto effectManager = levelSettings
                    ? levelSettings->m_effectManager
                    : nullptr;

                if (effectManager) {
                    auto objAction = ColorAction::create(
                        chosenColour,
                        false,
                        0
                    );

                    if (objAction) {
                        effectManager->setColorAction(objAction, 1004);
                        effectManager->updateColorAction(objAction);
                    }
                }
            }

            if (objects) {
                for (int i = static_cast<int>(objects->count()) - 1; i >= 0; i--) {
                    auto object = static_cast<GameObject*>(objects->objectAtIndex(i));

                    if (!object)
                        continue;

                    const bool teleportPortal =
                        isTeleportPortal(object->m_objectID);

                    if (!teleportPortal) {
                        const bool removeDecoration =
                            object->m_isDecoration ||
                            object->m_isDecoration2 ||
                            isVisualOnlyTrigger(object->m_objectID);

                        if (removeDecoration) {
                            m_editor->deleteObject(object, false);
                            continue;
                        }

                        if (object->m_isNoTouch) {
                            if (isNoTouchSolidOrHazard(object)) {
                                m_editor->deleteObject(object, false);
                            } else {
                                object->m_isInvisible = true;
                                object->m_isInvisibleBlock = true;
                                object->m_isHide = true;
                            }

                            continue;
                        }
                    }

                    object->m_isInvisible = false;
                    object->m_isInvisibleBlock = false;
                    object->m_isHide = false;

                    if (staticColour) {
                        if (object->m_baseColor) {
                            object->m_baseColor->m_hsv =
                                cchsv(0.f, 1.f, 1.f, false, false);
                            object->m_baseColor->m_usesHSV = false;
                        }

                        if (object->m_detailColor) {
                            object->m_detailColor->m_hsv =
                                cchsv(0.f, 1.f, 1.f, false, false);
                            object->m_detailColor->m_usesHSV = false;
                        }

                        object->setMainColorMode(1004);
                        object->setSecondaryColorMode(1004);

                        editorLayer->manualUpdateObjectColors(object);
                    }
                }
            }
        }

        delete this;
    }
};

class $modify(DecoDestroyerEditor, EditorUI) {
    void positionDecoDestroyerButton() {
        if (!m_deleteMenu)
            return;

        auto button = m_deleteMenu->getChildByTag(DECO_DESTROYER_TAG);

        if (!button)
            return;

        button->setPosition(-83.f, -18.f);
    }

    void setupDeleteMenu() {
        EditorUI::setupDeleteMenu();

        if (!m_deleteMenu)
            return;

        if (!m_deleteMenu->getChildByTag(DECO_DESTROYER_TAG)) {
            auto sprite = ButtonSprite::create(
                "DECO\nDESTROY",
                40,
                40,
                0.35f,
                true,
                "bigFont.fnt",
                "GJ_button_04.png",
                40
            );

            if (sprite) {
                auto button = CCMenuItemSpriteExtra::create(
                    sprite,
                    this,
                    menu_selector(DecoDestroyerEditor::onDestroyButton)
                );

                if (button) {
                    button->setTag(DECO_DESTROYER_TAG);
                    button->setSizeMult(1.2f);
                    m_deleteMenu->addChild(button);
                }
            }
        }

        positionDecoDestroyerButton();
    }

    void updateDeleteButtons() {
        EditorUI::updateDeleteButtons();
        positionDecoDestroyerButton();
    }

    void onDestroyButton(CCObject*) {
        auto delegate = new DecoDestroyerAlert(this);

        auto alert = FLAlertLayer::create(
            delegate,
            "Deco Destroyer",
            "Remove decoration and visual effects, delete no-touch solids and hazards, hide other no-touch objects, reveal hidden blocks, reset custom HSV, and recolour remaining objects?",
            "Cancel",
            "Destroy",
            360
        );

        if (alert) {
            alert->show();
        } else {
            delete delegate;
        }
    }
};
