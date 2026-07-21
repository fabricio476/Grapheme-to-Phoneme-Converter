# eSpeak NG - Conversor de Grafema para Fonema (G2P)

Um conversor compacto e de código aberto de **Grafema para Fonema (G2P)** baseado no [eSpeak NG](https://github.com/espeak-ng/espeak-ng). Este projeto foi simplificado ao remover toda a funcionalidade de síntese de áudio e reprodução de som, mantendo exclusivamente o motor principal de transcrição fonética.

Suporta **mais de 100 idiomas e sotaques**.

## Recursos

- **Conversão G2P:** Converte textos escritos em representações de fonemas.
- **Saída em IPA:** Transcrição no Alfabeto Fonético Internacional utilizando o parâmetro `--ipa`.
- **Mnemônicos de Fonemas:** Notação nativa de fonemas do eSpeak com `-x`.
- **Modo Trace (Rastreamento):** Depuração passo a passo da aplicação das regras linguísticas usando `-X`.
- **Separadores Customizáveis:** Defina caracteres separadores entre os fonemas (`--sep` e `--tie`).
- **Multi-idioma:** Suporte a dezenas de vozes/línguas com o parâmetro `-v` (por exemplo, `pt-br` para português brasileiro).
- **Compilação de Dicionários:** Permite compilar regras e dicionários de pronúncia personalizados com `--compile`.
- **Compacto e Independente:** Inclui todas as tabelas de fonemas pré-compiladas, sem necessidade de dependências externas complexas (~19MB no total).
- Desenvolvido em C.

## Como Compilar

### Requisitos

- CMake ≥ 3.8
- Compilador C (GCC ou Clang)

### Passos para Compilação

Para compilar o projeto em ambiente Linux, execute:

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

Durante a compilação, as tabelas de fonemas pré-compiladas serão copiadas automaticamente para o diretório `build`.

### Validar Instalação

Após compilar, você pode testar o conversor usando:

```bash
./build/src/espeak-ng --path=./build -v pt-br --ipa "Olá Mundo"
# Saída esperada: olˈa mˈũŋdʊ
```

## Como Usar

### Conversão Básica (Notação eSpeak)

```bash
./build/src/espeak-ng --path=build -v pt-br -x "Olá"
```

### Saída em Alfabeto Fonético Internacional (IPA)

```bash
./build/src/espeak-ng --path=build -v pt-br --ipa "Olá Mundo"
# olˈa mˈũŋdʊ

./build/src/espeak-ng --path=build -v en --ipa "Hello World"
# həlˈəʊ wˈɜːld
```

### IPA com Separadores de Fonemas

```bash
./build/src/espeak-ng --path=build -v pt-br --ipa --sep=" " "Olá"
# o l ˈa
```

### Modo Trace (Depuração de Regras)

```bash
./build/src/espeak-ng --path=build -v pt-br -X "Teste"
# Mostra detalhadamente a aplicação das regras de tradução
```

### A partir de um Arquivo

```bash
./build/src/espeak-ng --path=build -v pt-br --ipa -f entrada.txt
```

### A partir da Entrada Padrão (stdin)

```bash
echo "Olá" | ./build/src/espeak-ng --path=build -v pt-br --ipa --stdin
```

### Compilar Dicionário Customizado

Se você alterar as regras em `dictsource/`, compile o dicionário com:

```bash
./build/src/espeak-ng --path=build -v pt-br --compile
```

### Listar Vozes Disponíveis

```bash
./build/src/espeak-ng --path=build --voices
./build/src/espeak-ng --path=build --voices=pt    # Apenas vozes em português
```

## Opções da Linha de Comando

| Opção | Descrição |
| :--- | :--- |
| `-v <voz>` | Seleciona a voz/idioma (ex: `pt-br`, `en`, `fr`) |
| `-x` | Saída de fonemas na notação nativa do eSpeak |
| `-X` | Saída de fonemas acompanhada da depuração (trace) das regras |
| `--ipa` | Saída usando o Alfabeto Fonético Internacional (IPA) |
| `--sep=<char>` | Caractere para separar os fonemas gerados |
| `--tie=<char>` | Caractere de ligação para fonemas compostos por mais de uma letra |
| `-f <arquivo>` | Lê o texto de entrada de um arquivo |
| `--stdin` | Lê o texto de entrada do terminal (stdin) |
| `--path=<dir>` | Caminho para o diretório contendo a pasta `espeak-ng-data` |
| `--phonout=<file>` | Grava a saída fonética diretamente em um arquivo |
| `--compile` | Compila as regras e o dicionário de pronúncia |
| `--voices[=<lingua>]` | Lista todas as vozes ou filtra por idioma |
| `--version` | Exibe a versão do programa |
| `-h`, `--help` | Exibe a ajuda da linha de comando |

## Estrutura do Projeto

```text
Grapheme-to-Phoneme-Converter/
├── CMakeLists.txt          # Configuração de build
├── cmake/                  # Módulos adicionais do CMake
├── dictsource/             # Arquivos de origem dos dicionários (regras de pronúncia)
├── espeak-ng-data/         # Dicionários e dados de fonemas pré-compilados
│   ├── phontab             # Tabela de fonemas compilada
│   ├── phondata            # Dados de fonemas compilados
│   ├── *_dict              # Dicionários de idiomas compilados
│   ├── voices/             # Arquivos de definição de vozes/sotaques
│   └── lang/               # Definições específicas de idiomas
├── src/
│   ├── espeak-ng.c         # Aplicação principal (CLI)
│   └── libespeak-ng/       # Biblioteca principal do motor G2P
└── docs/                   # Documentações e tabelas de referência
```

## Caso de Uso & Motivação (ex: Piper TTS)

Este conversor é especialmente útil para motores de síntese de voz neurais modernos, como o **[Piper TTS](https://github.com/rhasspy/piper)**. 

Motores como o Piper utilizam o eSpeak NG apenas como um **fonetizador (phonemizer)** para traduzir o texto de entrada em IDs de fonemas antes de passá-los para a rede neural de síntese (VITS). Ao portar o Piper para plataformas móveis (como Android via NDK) ou embarcados, compilar o eSpeak NG original completo traz um grande volume de código de áudio desnecessário e dependências obsoletas.

Este repositório resolve esse problema fornecendo uma biblioteca G2P enxuta, permitindo compilações mais rápidas, arquivos binários finais menores e integração simplificada via JNI/Kotlin.

## Origem do Projeto

Este projeto é um fork do [eSpeak NG](https://github.com/espeak-ng/espeak-ng), modificado especificamente para atuar apenas como conversor de Grafema para Fonema (G2P). Toda e qualquer funcionalidade de síntese de áudio, geração de áudio (WAV) ou execução de som foram completamente removidas da base de código.


## Licença

Distribuído sob a licença [GNU GPL versão 3](COPYING) ou posterior.
