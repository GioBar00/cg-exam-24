
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
//        {SCENE_MAIN_MENU, "../scenes/main-menu.json"},
        {SceneId::SCENE_LEVEL_1, "../scenes/level-01.json"},
        // Future expansion
//      {SCENE_LEVEL_2, "Level 2"},
//      {SCENE_GAME_OVER, "Game Over"}
};

/* LEVEL SCENE OBJECTS */
enum class SceneObjectType {
    SO_PLAYER,
    SO_GROUND,
    SO_WALL,
    SO_LIGHT,
    SO_OTHER,
    // TODO: Add more scene objects depending on requirements (different shaders or interactions)
};