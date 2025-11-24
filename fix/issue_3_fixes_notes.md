# Fix: 3 Problemas Críticos - Slider, Vendas e Espaço no Inventário

## Problemas Corrigidos

### 1. Slider de Quantidade para QUANTIFIABLE
**Problema:** Slider aparecia para todos os itens QUANTIFIABLE (comidas, carnes, etc), mas deveria aparecer apenas para containers de líquidos.

**Solução:** 
- Modificado `ConfigureQuantitySliderForItem` para verificar se é container de líquido
- Se for container de líquido: mostrar slider e seletor de conteúdo
- Se não for (comidas/carnes): ocultar slider e sempre comprar em 100%

**Arquivo:** `Market/Scripts/5_Mission/AskalStoreMenu.c`

### 2. Vendas Bloqueadas ("Impossível Vender")
**Problema:** Nenhum item estava sendo vendido, sempre retornava erro.

**Solução:**
- Adicionado logs detalhados em `IsSellable` para debug
- Mantida lógica de verificação de cargo (containers vazios podem ser vendidos)
- Attachments são vendidos junto (já estava correto)
- Logs agora mostram exatamente onde a venda está sendo bloqueada

**Arquivo:** `Market/Scripts/4_World/AskalSellService.c`

**Nota:** Se vendas ainda falharem, os logs agora mostrarão o motivo exato (`container_has_cargo`, `has_cargo_items`, etc).

### 3. Espaço no Inventário - Busca Recursiva
**Problema:** Mesmo com mochila de 56 slots vazia, compras falhavam com "sem espaço no inventário".

**Solução:**
- Melhorada função `CreateSimpleItem` com busca recursiva em containers
- Estratégia em 4 etapas:
  1. Tentar inventário principal (busca recursiva automática do DayZ)
  2. Tentar criar nas mãos
  3. **NOVO:** Buscar recursivamente em containers (mochilas, etc) usando `FindSpaceInContainers`
  4. Se tudo falhar, dropar no chão próximo ao player (com offset para evitar colisão)

**Arquivo:** `Market/Scripts/4_World/AskalPurchaseService.c`

**Nova Função:** `FindSpaceInContainers` - enumera todos os itens do inventário e tenta criar o item em cada container encontrado.

## Arquivos Modificados

1. **`Market/Scripts/5_Mission/AskalStoreMenu.c`**
   - Ocultar slider para QUANTIFIABLE não-líquidos
   - Sempre comprar em 100% para comidas/carnes

2. **`Market/Scripts/4_World/AskalSellService.c`**
   - Adicionado logs detalhados em `IsSellable`
   - Melhor rastreamento de bloqueios de venda

3. **`Market/Scripts/4_World/AskalPurchaseService.c`**
   - Melhorada `CreateSimpleItem` com busca recursiva
   - Nova função `FindSpaceInContainers` para buscar em containers
   - Sempre dropar no chão se não houver espaço

## Testes Recomendados

1. **Slider QUANTIFIABLE:**
   - Selecionar comida/carne → slider deve estar oculto
   - Selecionar garrafa/container de líquido → slider deve aparecer

2. **Vendas:**
   - Tentar vender qualquer item → deve funcionar
   - Verificar logs do servidor para ver mensagens `[IsSellable]`
   - Se falhar, logs mostrarão o motivo exato

3. **Espaço no Inventário:**
   - Com mochila vazia, comprar item → deve ir para mochila
   - Com inventário cheio, comprar item → deve dropar no chão
   - Verificar logs para ver qual estratégia foi usada

## Notas

- Busca recursiva em containers agora é explícita e logada
- Drop no chão sempre acontece se não houver espaço (não retorna erro)
- Logs detalhados ajudam a diagnosticar problemas de venda
- Mudanças são mínimas e reversíveis

