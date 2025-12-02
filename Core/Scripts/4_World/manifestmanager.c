// ManifestManager (4_World) - lê o manifest.json se existir
class JsonManifestData
{
    bool Autodiscover;
}

class ManifestManager
{
    static bool ShouldAutoDiscover(string path = "$profile:Askal/Database/Datasets/manifest.json")
    {
        if (!FileExist(path)) return false;
        JsonManifestData manifest = new JsonManifestData();
        JsonFileLoader<JsonManifestData>.JsonLoadFile(path, manifest);
        if (manifest) return manifest.Autodiscover;
        return false;
    }
}