# Correção: Player Não Encontrado

## Problema Identificado

Nos logs do servidor, aparecia repetidamente:
```
[AskalPurchase] ❌ Player não encontrado
[AskalSell] [ERRO] Player não encontrado para RequestInventoryHealth
```

O método `GetPlayerFromIdentity()` não estava encontrando o player, mesmo com o SteamId sendo resolvido corretamente.

## Causa Raiz

O método estava usando apenas `GetWorld().GetPlayerList()` que pode não funcionar em todos os contextos ou momentos do ciclo de vida do servidor.

## Solução Implementada

### 1. **Múltiplas Estratégias de Busca**
Melhorei `GetPlayerFromIdentity()` em `AskalMarketHelpers.c` para usar 3 estratégias:

1. **Estratégia 1 (PRIMÁRIA)**: `GetPlayerObjectByIdentity()` - Método nativo do DayZ
   - Mais confiável e direto
   - Método oficial da API do DayZ

2. **Estratégia 2 (FALLBACK)**: `GetWorld().GetPlayerList()` - Lista de players conectados
   - Comparação por referência de `PlayerIdentity`
   - Mantido como fallback

3. **Estratégia 3 (FALLBACK ALTERNATIVO)**: Comparação por SteamId
   - Compara `GetPlainId()` de ambas as identities
   - Útil quando a referência de identity é diferente mas o SteamId é o mesmo

### 2. **Logs Detalhados**
Adicionei logs informativos para debug:
- Quantidade de players conectados
- Qual estratégia encontrou o player
- SteamId quando não encontra

### 3. **Correção de Warnings**
Corrigidos warnings de unsafe down-casting:
- `AskalCoreHelpers.c`: Usa `DayZGame.Cast()` em vez de cast direto
- `AskalMarketHelpers.c`: Usa `DayZGame.Cast()` em vez de cast direto

## Arquivos Modificados

1. `Market/Scripts/4_World/AskalMarketHelpers.c`
   - Método `GetPlayerFromIdentity()` completamente reescrito
   - Múltiplas estratégias de busca
   - Logs detalhados

2. `Core/Scripts/3_Game/AskalCoreHelpers.c`
   - Corrigido unsafe down-casting

## Testes Recomendados

1. Testar compra de itens normais
2. Testar compra de stackables (pregos, balas)
3. Testar venda de itens
4. Verificar logs para confirmar qual estratégia está funcionando

## Próximos Passos

Se ainda houver problemas:
1. Verificar logs para ver qual estratégia está sendo usada
2. Verificar se o player está realmente conectado quando a compra é feita
3. Considerar adicionar delay/retry se necessário

