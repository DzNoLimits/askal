class AskalCore
{
    protected static ref AskalCore s_Instance;
    protected ref AskalDatasetManager m_DatasetManager;

    static AskalCore GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new AskalCore();
            Print("[AskalCore] Core instance created.");
        }
        return s_Instance;
    }

    void AskalCore()
    {
        m_DatasetManager = new AskalDatasetManager();
        Print("[AskalCore] DatasetManager initialized.");
    }

    AskalDatasetManager GetDatasetManager()
    {
        return m_DatasetManager;
    }
}

// Função global para acesso fácil ao Core
static AskalCore GetAskalCore()
{
    return AskalCore.GetInstance();
}