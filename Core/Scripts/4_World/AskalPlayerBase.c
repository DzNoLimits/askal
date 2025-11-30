// AskalPlayerBase - Hook para carregar player config no connect
modded class PlayerBase
{
    override bool OnStoreLoadLifespan(ParamsReadContext ctx, int version)
    {
        bool result = super.OnStoreLoadLifespan(ctx, version);
        
        if (!result)
            return false;
        
        // Carregar config do player após OnStoreLoad
        PlayerIdentity identity = GetIdentity();
        if (identity)
        {
            // Use GetPlainId() to get Steam ID (numeric), not GetId() which returns Bohemia ID
            string steamId = identity.GetPlainId();
            if (!steamId || steamId == "")
            {
                // Fallback to GetId() only if GetPlainId() fails
                steamId = identity.GetId();
            }
            
            if (steamId && steamId != "")
            {
                // Chamar loader do Market (4_World context)
                AskalMarketHelpers.OnPlayerConnect(steamId);
            }
        }
        
        return true;
    }
}

