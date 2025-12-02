// ==========================================
// ActionConstructor - Registra UserActions do Askal
// ==========================================

modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        
        // Registrar ação de abrir menu do trader
        actions.Insert(ActionOpenAskalTraderMenu);
        
        Print("[AskalTrader] ✅ UserAction registrada: ActionOpenAskalTraderMenu");
    }
}

