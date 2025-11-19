# Implementação de Ajuste de Fracionamento para Stackables

## Resumo

Implementação de sistema melhorado para detectar e gerenciar itens stackables (empilháveis) no Market, baseado na lógica do COT (Community Online Tools).

## Arquivos Criados/Modificados

### 1. `Market/Scripts/4_World/AskalItemQuantityHelper.c` (NOVO)
Helper centralizado para detectar e gerenciar tipos de quantidade de itens:
- `DetectQuantityType()`: Detecta se item é NONE, MAGAZINE, STACKABLE ou QUANTIFIABLE
- `GetStackableRange()`: Obtém valores min/max para stackables (lê varQuantityMax do config)
- `GetMagazineRange()`: Obtém valores min/max para magazines (carregadores)
- `GetQuantifiableRange()`: Obtém valores min/max para quantifiables (líquidos, etc)
- `IsAmmunition()`: Verifica se é munição solta (prefixo "Ammo_")
- `IsMagazine()`: Verifica se é carregador de arma

### 2. `Market/Scripts/3_Game/AskalDatabaseStructures.c` (MODIFICADO)
- Movido enum `AskalItemQuantityType` para este arquivo (compartilhado)

### 3. `Market/Scripts/5_Mission/AskalStoreMenu.c` (MODIFICADO)
- `DetectItemQuantityType()`: Agora usa `AskalItemQuantityHelper.DetectQuantityType()`
- `GetQuantityRangeForItem()`: Agora usa os métodos do helper para obter ranges

## Lógica de Detecção

### Prioridade 1: Magazines (Carregadores)
- Se é `Magazine` e tem `GetAmmoMax() > 0` e não é `IsSplitable()` → **MAGAZINE**
- Se é `Magazine` e é `IsSplitable()` ou tem prefixo "Ammo_" → **STACKABLE**

### Prioridade 2: Containers de Líquido
- Se `IsLiquidContainer()` → **QUANTIFIABLE**

### Prioridade 3: Stackables Genéricos
- Se `HasQuantity()` e `IsSplitable()` → **STACKABLE**
- Se `HasQuantity()` mas não é `IsSplitable()` → **QUANTIFIABLE**

## Leitura de Configuração

### Para Munições (Ammo_*)
1. Tenta ler `CfgMagazines <className> count`
2. Fallback: usa `GetQuantityMax()` do objeto

### Para Stackables Genéricos (ex: Nail)
1. Tenta ler `CfgVehicles <className> varQuantityMax`
2. Tenta ler `CfgVehicles <className> varQuantityMin`
3. Fallback: usa `GetQuantityMax()` e `GetQuantityMin()` do objeto

### Para Magazines
1. Usa `GetAmmoMax()` do objeto
2. Para `IsAmmoPile()` com max > 1, mínimo é 1

## Exemplos

### Pregos (Nail)
- Config: `varQuantityMax = 99.0`, `varQuantityMin = 0.0`, `canBeSplit = 1`
- Tipo detectado: **STACKABLE**
- Range: 0-99 unidades
- Step: 1.0 (inteiros)

### Munição Solta (Ammo_45ACP)
- Config: `count = 25`, `canBeSplit = 1`
- Tipo detectado: **STACKABLE**
- Range: 1-25 unidades (se IsAmmoPile) ou 0-25
- Step: 1.0 (inteiros)

### Carregador (Mag_FNX45_15Rnd)
- Config: `count = 15`, `ammo = "Bullet_45ACP"`
- Tipo detectado: **MAGAZINE**
- Range: 0-15 balas
- Step: 1.0 (inteiros)

## Próximos Passos

1. ✅ Criar helper centralizado
2. ✅ Mover enum para arquivo compartilhado
3. ✅ Atualizar AskalStoreMenu para usar helper
4. ⏳ Testar detecção com diferentes tipos de itens
5. ⏳ Verificar cálculo de preço por unidade
6. ⏳ Garantir que slider funcione corretamente no UI

## Notas Técnicas

- Baseado na lógica do COT (`JMObjectSpawnerForm.c` e `JMObjectSpawnerModule.c`)
- Usa criação temporária de objetos para inspeção (como o COT faz)
- Lê valores do config quando disponível, fallback para valores do objeto
- Diferenciação crítica entre munições soltas e carregadores de arma

