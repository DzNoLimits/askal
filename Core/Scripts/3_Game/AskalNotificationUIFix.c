// ==========================================
// Fix para NotificationUI::UpdateOffset
// Corrige o erro de NULL pointer quando notification_root não existe
// ==========================================

modded class NotificationUI
{
    // Sobrescreve UpdateOffset para verificar NULL antes de usar
    override void UpdateOffset()
    {
        UIScriptedMenu menu = UIScriptedMenu.Cast(GetGame().GetUIManager().GetMenu());
        if (!menu)
        {
            // Se não há menu, restaurar posição original se necessário
            if (m_OffsetEnabled)
            {
                m_Root.SetPos(m_BackupPosX, m_BackupPosY);
                m_OffsetEnabled = false;
            }
            return;
        }
        
        // Verificar se GetLayoutRoot retorna NULL
        Widget layoutRoot = menu.GetLayoutRoot();
        if (!layoutRoot)
        {
            // Layout root não existe ainda - restaurar posição se necessário
            if (m_OffsetEnabled)
            {
                m_Root.SetPos(m_BackupPosX, m_BackupPosY);
                m_OffsetEnabled = false;
            }
            return;
        }
        
        Widget expNotification = layoutRoot.FindAnyWidget("notification_root");
        if (!expNotification)
        {
            // Widget não existe - restaurar posição original se necessário
            if (m_OffsetEnabled)
            {
                m_Root.SetPos(m_BackupPosX, m_BackupPosY);
                m_OffsetEnabled = false;
            }
            return;
        }
        
        // Widget existe - continuar com lógica original
        if (expNotification.IsVisible())
        {
            if (!m_OffsetEnabled)
            {
                m_Root.GetPos(m_BackupPosX, m_BackupPosY);

                float x, y, w, h;
                m_Root.GetScreenPos(x, y);
                expNotification.GetScreenSize(w, h);

                m_Root.SetScreenPos(x, h);
                m_OffsetEnabled = true;
            }
        }
        else if (m_OffsetEnabled)
        {
            m_Root.SetPos(m_BackupPosX, m_BackupPosY);
            m_OffsetEnabled = false;
        }
    }
}

