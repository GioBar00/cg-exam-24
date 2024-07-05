
/* SCENE DEFINITIONS */
enum class SceneId {
    SCENE_MAIN_MENU,
    SCENE_LEVEL_1,
    // Future expansion
//    SCENE_LEVEL_2,
//    SCENE_GAME_OVER
};

static const SceneId sceneIds[] = {
        SceneId::SCENE_MAIN_MENU,
        SceneId::SCENE_LEVEL_1,
        // Future expansion
//        SceneId::SCENE_LEVEL_2,
//        SceneId::SCENE_GAME_OVER
};

typedef std::unordered_map<SceneId, std::string> SceneIdMap_t;

// TODO: Add the scene files
const SceneIdMap_t sceneFiles = {
//        {SCENE_MAIN_MENU, "/scenes/main_menu.json"},
        {SceneId::SCENE_LEVEL_1, "/scenes/level_1.json"},
        // Future expansion
//      {SCENE_LEVEL_2, "Level 2"},
//      {SCENE_GAME_OVER, "Game Over"}
};
