# Release 3.1 — Auditoria de Segurança AMD64

## Escopo auditado

A auditoria concentrou-se nos caminhos reais do ReactOS para dispatcher de syscalls AMD64, cópias sob SMAP, inicialização de NXE/DEP, liberação de pool e validação estrutural de imagens PE32+.

## Estado encontrado

| Área | Estado observado | Impacto |
|---|---|---|
| Dispatcher AMD64 | `ntoskrnl/ke/amd64/traphandler.c` já rejeita `UserRsp > MmUserProbeAddress` e valida a faixa de parâmetros com aritmética bounded. | O incremento de Release 3.1 deve fechar canonicalidade, alinhamento e limites superiores sem reescrever a entrada assembly. |
| SMAP | `MiSmapCopyMemory` em `ntoskrnl/mm/ARM3/virtual.c` já usa STAC/CLAC dentro de `_SEH2_FINALLY`; os caminhos `MiDoMappedCopy`/`MiDoPoolCopy` reutilizam o helper. | Evitar duplicação. Auditar overflow de `Length` e manter SMAP global desligado enquanto todos os acessos não tiverem a mesma garantia. |
| NXE/DEP | `ntoskrnl/ke/amd64/kiinit.c` já verifica `KF_NX_BIT`, configura `EFER_NXE`, publica `NXSupportPolicy` e preserva NXE ao habilitar `MSR_SCE`. | A alteração deve validar leitura/escrita do MSR e não anunciar NX quando a CPU não suporta a feature. |
| Pool | `ExpSanitizeFreedPool` preenche com `0xDD` somente em `DBG`; `ExFreePoolWithTag` chama o helper para big pages e payload normal antes da reutilização/coalescência. | O requisito “checked build” já está implementado. Só são necessárias validações de tamanho/underflow e autoria Release 3.1 se o arquivo for modificado. |
| CI/PE32+ | `drivers/crypto/ci/ci.c` já valida DOS/NT signature, AMD64, PE32+, número de seções, tamanho opcional, tabela de seções, headers e image size. `DriverEntry` retorna `STATUS_NOT_SUPPORTED` porque o loader não chama o boundary. | O escopo seguro é reforçar overflow/conversões e manter explícito que não há Authenticode, catálogo ou política de confiança. Não conectar ao loader sem contrato testado. |

## Política de implementação

Não será implementada uma nova camada paralela de syscall, uma política global de SMAP, um loader de confiança ou um HVCI fictício. Cada alteração deve preservar códigos de erro explícitos, manter o boot padrão e ser compilada antes do módulo seguinte. Todos os arquivos alterados receberão o marcador `Release 3.1 contribution by Liandro Lopes <anjoleandrolopes@gmail.com>.`, preservando os créditos históricos.

## Alvos imediatos

1. Endurecer `KiSystemCallHandler` com validação canonical de `UserRsp`, alinhamento compatível e checagem de overflow da faixa de parâmetros.
2. Revalidar o helper `MiSmapCopyMemory` sem duplicar STAC/CLAC e sem ativar SMAP global.
3. Acrescentar checks defensivos ao caminho já existente de NXE e ao poisoning de pool apenas onde não alterar a semântica de produção.
4. Reforçar o validador PE32+ com aritmética de faixa e limites de `SizeOfImage`, sem declarar verificação criptográfica.
