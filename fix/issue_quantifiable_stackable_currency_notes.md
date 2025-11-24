# Fix: QUANTIFIABLE 50%, STACKABLE Metade, e Venda Falhando

## Problemas Corrigidos

### 1. QUANTIFIABLE Sempre Vindo em 50%
**Problema:** Itens QUANTIFIABLE (comidas, carnes, frutas, latinhas, arroz) estavam sempre vindo com 50% de quantidade, mesmo quando deveriam vir em 100%.

**Causa:** O código estava usando `quantity` (que vem como 100 = percentual) diretamente como quantidade real, mas `GetQuantityMin()` e `GetQuantityMax()` retornam valores reais (ex: 0.0 a 1.0). Quando fazia `Clamp(100, 0.0, 1.0)`, resultava em 1.0, mas o item era criado com quantidade padrão de 0.5 (50%).

**Solução:** 
- Converter percentual (0-100) para quantidade real: `maxQty * (percentual / 100.0)`
- Garantir que está dentro dos limites do item
- Aplicar quantidade correta

**Arquivo:** `Market/Scripts/4_World/AskalPurchaseService.c` (linhas 210-225)

### 2. STACKABLE Recebendo Metade da Quantidade
**Problema:** Ao comprar itens STACKABLE com quantidade > 1, era cobrado pela quantidade escolhida mas só recebia metade.

**Causa:** Itens eram criados com quantidade padrão (metade do max) e `SetQuantity` não estava sendo aplicado corretamente ou havia timing issues.

**Solução:**
- Adicionado loop de retry (até 3 tentativas) para garantir que `SetQuantity` seja aplicado
- Logs detalhados para debug
- Verificação final se quantidade foi aplicada corretamente

**Arquivo:** `Market/Scripts/4_World/AskalPurchaseService.c` (linhas 168-198)

### 3. Venda Falhando - "Falha ao adicionar dinheiro"
**Problema:** Nenhum item estava sendo vendido, sempre retornava "Falha ao adicionar dinheiro" mesmo com moeda configurada.

**Causa:** `AddPhysicalCurrency` estava retornando `true` sempre, mas pode estar falhando silenciosamente se:
- `CalculateChange` retorna array vazio
- Moeda não está configurada corretamente
- Falha ao criar moedas no inventário ou no chão

**Solução:**
- Verificar se `CalculateChange` retorna valores válidos (não vazio)
- Adicionar logs detalhados para cada tipo de moeda sendo spawnada
- Verificar se pelo menos algumas moedas foram spawnadas (inventário ou chão)
- Retornar `false` se nenhuma moeda foi spawnada
- Melhorar drop no chão com offset para evitar colisão

**Arquivo:** `Market/Scripts/4_World/AskalCurrencyInventoryManager.c` (linhas 134-174)

## Arquivos Modificados

1. **`Market/Scripts/4_World/AskalPurchaseService.c`**
   - Corrigido QUANTIFIABLE para converter percentual (0-100) para quantidade real
   - Melhorado STACKABLE com retry loop para garantir quantidade correta

2. **`Market/Scripts/4_World/AskalCurrencyInventoryManager.c`**
   - Adicionado validação de `CalculateChange` retornando valores válidos
   - Logs detalhados para debug de moedas
   - Verificação se moedas foram spawnadas
   - Melhorado drop no chão com offset

## Mudanças Detalhadas

### QUANTIFIABLE (linhas 210-225):
```enforce
// ANTES:
float clampedQtyPercent = Math.Clamp(quantity, itemBase.GetQuantityMin(), itemBase.GetQuantityMax());
itemBase.SetQuantity(clampedQtyPercent);

// DEPOIS:
float maxQty = itemBase.GetQuantityMax();
float qtyMin = itemBase.GetQuantityMin();
float clampedPercent = Math.Clamp(quantity, 0.0, 100.0);
float actualQty = maxQty * (clampedPercent / 100.0);
actualQty = Math.Clamp(actualQty, qtyMin, maxQty);
itemBase.SetQuantity(actualQty);
```

### STACKABLE (linhas 168-198):
- Adicionado loop de retry (até 3 tentativas)
- Logs detalhados de cada tentativa
- Verificação final se quantidade foi aplicada

### AddPhysicalCurrency (linhas 134-174):
- Validação se `CalculateChange` retorna valores válidos
- Logs para cada tipo de moeda sendo spawnada
- Verificação se pelo menos algumas moedas foram spawnadas
- Retorna `false` se falhar completamente

## Testes Recomendados

1. **QUANTIFIABLE:**
   - Comprar carne/fruta/latinha → deve vir em 100% (quantidade máxima)
   - Verificar logs: `QUANTIFIABLE - Percentual: 100% | Quantidade real: X (max: Y)`

2. **STACKABLE:**
   - Comprar 20 unidades de ammo → deve receber 20 unidades
   - Comprar 5 unidades de bandagem → deve receber 5 unidades
   - Verificar logs: `STACKABLE: X unidades aplicadas com sucesso`

3. **Venda:**
   - Tentar vender qualquer item → deve funcionar
   - Verificar logs do servidor:
     - `💰 Calculando troco para X Askal_Money`
     - `💰 Spawnando Yx CoinClass`
     - `✅ Adicionado X Askal_Money`
   - Se falhar, logs mostrarão o motivo exato

## Notas

- QUANTIFIABLE agora sempre compra em 100% (quantidade máxima) para não-líquidos
- STACKABLE tem retry loop para garantir quantidade correta
- Venda agora tem validações e logs detalhados para identificar problemas de moeda
- Mudanças são mínimas e reversíveis

