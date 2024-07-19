
/* SCENE DEFINITIONS */
enum class SceneId {
    SCENE_MAIN_MENU,
    SCENE_LEVEL_1,
    SCENE_LEVEL_2,
    SCENE_GAME_OVER
};

static const SceneId sceneIds[] = {
        SceneId::SCENE_MAIN_MENU,
        SceneId::SCENE_LEVEL_1,
        SceneId::SCENE_LEVEL_2,
        SceneId::SCENE_GAME_OVER
};

typedef std::unordered_map<SceneId, std::string> SceneIdMap_t;

// TODO: Add the scene files
const SceneIdMap_t sceneFiles = {
        {SceneId::SCENE_MAIN_MENU, "Main Menu"},
        {SceneId::SCENE_LEVEL_1,   "../scenes/level-01.json"},
        {SceneId::SCENE_LEVEL_2,   "../scenes/level-02.json"},
        {SceneId::SCENE_GAME_OVER, "Game Over"}
};

/* LEVEL SCENE OBJECTS */
enum class SceneObjectType {
    // LevelScene
    SO_PLAYER,
    SO_GROUND,
    SO_WALL,
    SO_LIGHT,
    SO_TORCH,
    SO_LAMP,
    SO_BONFIRE,
    SO_TRAPDOOR,
    // ScreenScene
    SO_BUTTON,
    SO_CURSOR,
    // Others
    SO_OTHER,
};

std::map<SceneObjectType, std::string> levelSceneObjectTypes = {
        {SceneObjectType::SO_PLAYER,   "SO_PLAYER"},
        {SceneObjectType::SO_GROUND,   "SO_GROUND"},
        {SceneObjectType::SO_WALL,     "SO_WALL"},
        {SceneObjectType::SO_LIGHT,    "SO_LIGHT"},
        {SceneObjectType::SO_TORCH,    "SO_TORCH"},
        {SceneObjectType::SO_LAMP,     "SO_LAMP"},
        {SceneObjectType::SO_BONFIRE,  "SO_BONFIRE"},
        {SceneObjectType::SO_TRAPDOOR, "SO_TRAPDOOR"},
        {SceneObjectType::SO_OTHER,    "SO_OTHER"}
};