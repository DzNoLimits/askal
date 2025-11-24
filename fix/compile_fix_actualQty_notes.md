# Fix: Multiple Declaration of Variable 'actualQty'

## Erro
```
Askal/Market/Scripts/4_World/askalpurchaseservice.c(229): Multiple declaration of variable 'actualQty'
```

## Causa
A variável `actualQty` foi declarada duas vezes no mesmo escopo da função `CreateSimpleItem`:
- Linha 185: dentro do bloco `else if (quantityType == 2 && itemBase.HasQuantity()) // STACKABLE`
- Linha 229: dentro do bloco `else if (itemBase.HasQuantity())` dentro do `else if (quantityType == 3 && itemBase.HasQuantity()) // QUANTIFIABLE`

Em EnforceScript, variáveis declaradas em blocos `if/else` ainda estão no mesmo escopo da função, causando conflito.

## Solução
Renomeada a segunda declaração de `actualQty` para `actualQty_quantifiable` para evitar conflito.

## Arquivo Modificado
- `Market/Scripts/4_World/AskalPurchaseService.c` (linha 229)

## Mudança
```enforce
// ANTES:
float actualQty = maxQty * (clampedPercent / 100.0);
actualQty = Math.Clamp(actualQty, qtyMin_quantifiable, maxQty);
itemBase.SetQuantity(actualQty);
Print("[AskalPurchase] 📊 QUANTIFIABLE - Percentual: " + clampedPercent + "% | Quantidade real: " + actualQty + " (max: " + maxQty + ")");

// DEPOIS:
float actualQty_quantifiable = maxQty * (clampedPercent / 100.0);
actualQty_quantifiable = Math.Clamp(actualQty_quantifiable, qtyMin_quantifiable, maxQty);
itemBase.SetQuantity(actualQty_quantifiable);
Print("[AskalPurchase] 📊 QUANTIFIABLE - Percentual: " + clampedPercent + "% | Quantidade real: " + actualQty_quantifiable + " (max: " + maxQty + ")");
```

## Status
✅ Corrigido - Compilação deve funcionar agora

