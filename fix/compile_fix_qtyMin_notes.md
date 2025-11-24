# Fix: Multiple Declaration of Variable 'qtyMin'

## Erro
```
Askal/Market/Scripts/4_World/askalpurchaseservice.c(223): Multiple declaration of variable 'qtyMin'
```

## Causa
A variável `qtyMin` foi declarada duas vezes no mesmo escopo da função `CreateSimpleItem`:
- Linha 171: dentro do bloco `else if (quantityType == 2 && itemBase.HasQuantity()) // STACKABLE`
- Linha 223: dentro do bloco `else if (itemBase.HasQuantity())` dentro do `else if (quantityType == 3 && itemBase.HasQuantity()) // QUANTIFIABLE`

Em EnforceScript, variáveis declaradas em blocos `if/else` ainda estão no mesmo escopo da função, causando conflito.

## Solução
Renomeada a segunda declaração de `qtyMin` para `qtyMin_quantifiable` para evitar conflito.

## Arquivo Modificado
- `Market/Scripts/4_World/AskalPurchaseService.c` (linha 223)

## Mudança
```enforce
// ANTES:
float qtyMin = itemBase.GetQuantityMin();
actualQty = Math.Clamp(actualQty, qtyMin, maxQty);

// DEPOIS:
float qtyMin_quantifiable = itemBase.GetQuantityMin();
actualQty = Math.Clamp(actualQty, qtyMin_quantifiable, maxQty);
```

## Status
✅ Corrigido - Compilação deve funcionar agora

