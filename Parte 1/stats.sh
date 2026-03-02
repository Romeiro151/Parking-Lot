#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº: 129838       Nome: José Miguel Oliveira Romeiro
## Nome do Módulo: S3. Script: stats.sh
## Descrição/Explicação do Módulo:
##  
##  Neste módulo, escrevi num ficheiro de estatísticas em html.
##  Comecei por verificar se consigo ler ou escrever nos ficheiros paises.txt e estacionamentos.txt
##  Verifiquei também com um for e um if para ver se todos os argumentos passados ao script
##  são válidos ou não (número entre 1 e 7).
##  Crio uma variável para fazer uma lista de todos os arquivos fo formato arquivo-*.park
##  e caso não encontrar nenhum ou encontrar erros redireciona para /dev/null. De seguida 
##  verifico com um for e um if se nenhum dos arquivos nessa lista criada há algum arquivo-<Ano>-<Mês>
##  e, também, verifico se caso exista esse ficheiro se é possível ler-lo.
##
#####################################################################################

## Este script obtém informações sobre o sistema Park-IUL, afixando os resultados das estatísticas pedidas no formato standard HTML no Standard Output e no ficheiro stats.html. Cada invocação deste script apaga e cria de novo o ficheiro stats.html, e poderá resultar em uma ou várias estatísticas a serem produzidas, todas elas deverão ser guardadas no mesmo ficheiro stats.html, pela ordem que foram especificadas pelos argumentos do script.

## S3.1. Validações:
## O script valida se, na diretoria atual, existe algum ficheiro com o nome arquivo-<Ano>-<Mês>.park, gerado pelo Script: manutencao.sh. Se não existirem ou não puderem ser lidos, dá so_error S3.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S3.1.

for arg in "$@"; do
    if ! [[ "$arg" =~ ^[1-7]$ ]]; then
        so_error S3.1 "Argumento inválido: $arg"
        exit 1
    fi
done

arquivos=$(ls arquivo-*.park 2>/dev/null)
if [[ -z "$arquivos" ]]; then
    so_error S3.1 "Nenhum ficheiro arquivo-<Ano>-<Mês>.park encontrado!"
    exit 1
fi

for arquivo in $arquivos; do
    if [[ ! -r "$arquivo" ]]; then
        so_error S3.1 "Não se consegue ler o ficheiro $arquivo"
        exit 1
    fi
done

if [[ ! -r "paises.txt" ]]; then
    so_error S3.1 "Não existe ou não pode ser lido o ficheiro paises.txt"
    exit 1
fi

if [[ ! -r "estacionamentos.txt" ]]; then
    so_error S3.1 "Não existe ou não pode ser lido o ficheiro estacionamentos.txt"
    exit 1
fi

so_success S3.1





## S3.2. Estatísticas:
## Cada uma das estatísticas seguintes diz respeito à extração de informação dos ficheiros do sistema Park-IUL. Caso não haja informação suficiente para preencher a estatística, poderá apresentar uma lista vazia.
## S3.2.1.  Obter uma lista das matrículas e dos nomes de todos os condutores cujas viaturas estão ainda estacionadas no parque, ordenados alfabeticamente por nome de condutor:
## <h2>Stats1:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>Condutor:</b> <Nome do Condutor></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Condutor:</b> <Nome do Condutor></li>
## ...
## </ul>

echo "<h2>Stats1:</h2>" >> stats.html
echo "<ul>" >> stats.html

ficheiro_temporario=$(mktemp)
sorted_file=$(mktemp)

IFS=$'\n'
for linha in $( cat "estacionamentos.txt" ); do
    matricula=$( echo $linha | awk -F':' '{print $1}' )
    nome_condutor=$( echo $linha | awk -F':' '{print $4}')
    
    echo "$nome_condutor:$matricula" >> "ficheiro_temporario"
done

sort -t':' -k1,1 "ficheiro_temporario" | uniq | head -4 > "sorted_file"

IFS=$'\n'
for linha in $( cat "sorted_file" ); do
    condutor=$( echo $linha | awk -F':' '{print $1}' )
    matricula=$( echo $linha | awk -F':' '{print $2}' )

    echo "<li><b>Matrícula:</b> $matricula <b>Condutor:</b> $condutor </li>" >> stats.html
done

rm "ficheiro_temporario" "sorted_file"

echo "</ul>" >> stats.html
    
## S3.2.2. Obter uma lista do top3 das matrículas e do tempo estacionado das viaturas que já terminaram o estacionamento e passaram mais tempo estacionadas, ordenados decrescentemente pelo tempo de estacionamento (considere apenas os estacionamentos cujos tempos já foram calculados):
## <h2>Stats2:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## </ul>



## S3.2.3. Obter as somas dos tempos de estacionamento das viaturas que não são motociclos, agrupadas pelo nome do país da matrícula (considere apenas os estacionamentos cujos tempos já foram calculados):
## <h2>Stats3:</h2>
## <ul>
## <li><b>País:</b> <Nome País> <b>Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>País:</b> <Nome País> <b>Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>



## S3.2.4. Listar a matrícula, código de país e data de entrada dos 3 estacionamentos, já terminados ou não, que registaram uma entrada mais tarde (hora de entrada) no parque de estacionamento, ordenados crescentemente por hora de entrada:
## <h2>Stats4:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## </ul>


## S3.2.5. Tendo em consideração que um utilizador poderá ter várias viaturas, determine o tempo total, medido em dias, horas e minutos gasto por cada utilizador da plataforma (ou seja, agrupe os minutos em dias e horas).
## <h2>Stats5:</h2>
## <ul>
## <li><b>Condutor:</b> <NomeCondutor> <b>Tempo  total:</b> <x> dia(s), <y> hora(s) e <z> minuto(s)</li>
## <li><b>Condutor:</b> <NomeCondutor> <b>Tempo  total:</b> <x> dia(s), <y> hora(s) e <z> minuto(s)</li>
## ...
## </ul>



## S3.2.6. Liste as matrículas das viaturas distintas e o tempo total de estacionamento de cada uma, agrupadas pelo nome do país com um totalizador de tempo de estacionamento por grupo, e totalizador de tempo global.
## <h2>Stats6:</h2>
## <ul>
## <li><b>País:</b> <Nome País></li>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>
## <li><b>País:</b> <Nome País></li>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>
## ...
## </ul>



## S3.2.7. Obter uma lista do top3 dos nomes mais compridos de condutores cujas viaturas já estiveram estacionadas no parque (ou que ainda estão estacionadas no parque), ordenados decrescentemente pelo tamanho do nome do condutor:
## <h2>Stats7:</h2>
## <ul>
## <li><b> Condutor:</b> <Nome do Condutor mais comprido></li>
## <li><b> Condutor:</b> <Nome do Condutor segundo mais comprido></li>
## <li><b> Condutor:</b> <Nome do Condutor terceiro mais comprido></li>
## </ul>

temp_file=$(mktemp)  

echo "<h2>Stats7:</h2>" >> stats.html
echo "<ul>"

IFS=$'\n'
for linha in $( cat "estacionamentos.txt" ); do
    
    nome_condutor=$( echo $linha | awk -F':' '{print $4}' )
    primeiro_nome=$( echo $nome_condutor | cut -d' ' -f1 )
    ultimo_nome=$( echo $nome_condutor | cut -d' ' -f2 )
    nomes=$( cut -d':' -f5 _etc_passwd | sed 's/,*$//g' )  
    nome_total=$( echo $nomes | grep -q "^$primeiro_nome.*$ultimo_nome" ) 
    tamanho_nome=$( echo $nome_total | wc -w )
    echo "$tamanho_nome:$nome_condutor" >> "$temp_file"
done

sort -t':' -k1,1nr "$temp_file" | head -3 | while IFS=: read -r tamanho nome; do
    echo "<li><b>Condutor:</b> $nome</li>" >> stats.html
done

rm "$temp_file"
echo "</ul>"



## S3.3. Processamento do script:
## S3.3.1. O script cria uma página em formato HTML, chamada stats.html, onde lista as várias estatísticas pedidas.
## O ficheiro stats.html tem o seguinte formato:
## <html><head><meta charset="UTF-8"><title>Park-IUL: Estatísticas de estacionamento</title></head>
## <body><h1>Lista atualizada em <Data Atual, formato AAAA-MM-DD> <Hora Atual, formato HH:MM:SS></h1>
## [html da estatística pedida]
## [html da estatística pedida]
## ...
## </body></html>
## Sempre que o script for chamado, deverá:
## • Criar o ficheiro stats.html.
## • Preencher, neste ficheiro, o cabeçalho, com as duas linhas HTML descritas acima, substituindo os campos pelos valores de data e hora pelos do sistema.
## • Ciclicamente, preencher cada uma das estatísticas pedidas, pela ordem pedida, com o HTML correspondente ao indicado na secção S3.2.
## • No final de todas as estatísticas preenchidas, terminar o ficheiro com a última linha “</body></html>”
