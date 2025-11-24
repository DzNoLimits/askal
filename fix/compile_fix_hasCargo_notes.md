# Fix: Multiple Declaration of Variable 'hasCargo'

## Erro
```
Askal/Market/Scripts/4_World/askalsellservice.c(302): Multiple declaration of variable 'hasCargo'
```

## Causa
A variável `hasCargo` foi declarada duas vezes no mesmo escopo da função `IsSellable`:
- Linha 285: dentro do bloco `if (container)`
- Linha 302: dentro do bloco `if (item.GetInventory())`

Em EnforceScript, variáveis declaradas em blocos `if` ainda estão no mesmo escopo da função, causando conflito.

## Solução
Renomeada a segunda declaração de `hasCargo` para `hasCargoItems` para evitar conflito.

## Arquivo Modificado
- `Market/Scripts/4_World/AskalSellService.c` (linha 302)

## Mudança
```enforce
// ANTES:
bool hasCargo = HasCargoItemsRecursive(item);

// DEPOIS:
bool hasCargoItems = HasCargoItemsRecursive(item);
```

## Status
✅ Corrigido - Compilação deve funcionar agora

