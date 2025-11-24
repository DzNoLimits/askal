# Fix: Vendas e Compras Bloqueadas

## Problemas Identificados

### 1. Vendas Bloqueadas por Attachments
**Erro:** Itens com attachments (ex: `M68Optic` com `Battery`) estavam sendo bloqueados para venda.

**Causa:** A função `IsSellable` estava bloqueando itens que tinham attachments do tipo "Battery", "Mag_", ou "Scope_", mas attachments devem ser vendidos junto com o item.

**Solução:** Removido o bloqueio de attachments. Attachments fazem parte do item e devem ser vendidos junto.

### 2. Compras Falhando com `invalid_quantity`
**Erro:** Compras de ammo stackable (ex: `ammo_762x54` com quantidade 5 ou 9) estavam sendo rejeitadas com `invalid_quantity`.

**Causa:** A função `ValidateQuantity` estava verificando `item.HasQuantity()` para stackables, mas alguns tipos de ammo podem não ter `HasQuantity()` retornando valores válidos quando criados temporariamente. A validação precisava de fallback para config.

**Solução:** 
- Melhorada a validação para stackables: verifica `GetQuantityMax()` primeiro
- Adicionado fallback para ler `count` do config `CfgMagazines` quando `HasQuantity()` não retorna valores válidos
- Adicionado fallback permissivo para ammo sem config (para evitar bloqueios desnecessários)

## Arquivos Modificados

1. **`Market/Scripts/4_World/AskalSellService.c`**
   - Removido bloqueio de attachments na função `IsSellable`
   - Attachments agora são vendidos junto com o item

2. **`Market/Scripts/4_World/AskalPurchaseService.c`**
   - Melhorada função `ValidateQuantity` para tratar melhor ammo stackable
   - Adicionado fallback para config quando `HasQuantity()` não retorna valores válidos
   - Melhorada detecção de Magazine vs Stackable

## Testes Recomendados

1. **Venda de itens com attachments:**
   - Tentar vender `M68Optic` com `Battery` → deve funcionar
   - Tentar vender armas com scopes/magazines → devem funcionar

2. **Compra de ammo stackable:**
   - Comprar `ammo_762x54` com quantidade 5 → deve funcionar
   - Comprar `ammo_762x54` com quantidade 9 → deve funcionar
   - Comprar `ammo_762x54` com quantidade 20 (max) → deve funcionar

## Notas

- Attachments são parte integrante do item e devem ser vendidos junto
- A validação de quantidade agora é mais robusta com múltiplos fallbacks
- Mudanças são mínimas e reversíveis

