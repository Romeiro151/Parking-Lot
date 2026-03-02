#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº: 129838       Nome: José Miguel Oliveira Romeiro
## Nome do Módulo: S2. Script: manutencao.sh
## Descrição/Explicação do Módulo:
##
##	Neste módulo, realiza-se a manutenção do ficheiro estacionamentos.txt.
## 	Inicialmente corro o ficheiro estacionamentos.txt e verfico se há
## 	algum registo que não esteja de acordo com as regras que verifiquei 
##  no regista_passagem.sh e verifico também se alguma linha do ficheiro
## 	tem uma data de entrada superior à de saída. Para conseguir verificar isto,
## 	uma vez que já não estou a analisar uma informação que entra no terminal,
##	mas sim informações de um ficheiro, utilizamos um for loop que percorre todas
## 	as linhas do ficheiro estacionamento, para assim puder acessar diretamente
## 	certas informações. Dentro do for loop utilizo estruturas if e variáveis com 
## 	grep's, awk's e sed's.
## 	Por fim, procuro todas as linhas que estjam completas, isto é, que tenham
## 	registo de saída e registo de entrada. Caso certa linha esteja completa movi
## 	essa linha do ficheiro  para um novo ficheiro de nome arquivo-<Ano>-<Mes>.park.
##	Nesse ficheiro adiciona-se também um último argumeto que trata do tempo que o veiculo esteve
##	no ficheiro estacionamentos.txt (tempo_park_minutos).
##  Começo sempre por verificar se o ficheiro estacionamentos existe, bem como o paises.txt
##  Verifico se estacionamentos.txt está vazio.
##
#####################################################################################

## Este script não recebe nenhum argumento, e permite realizar a manutenção dos registos de estacionamento. 

## S2.1. Validações do script:
## O script valida se, no ficheiro estacionamentos.txt:
## • Todos os registos referem códigos de países existentes no ficheiro paises.txt;
## • Todas as matrículas registadas correspondem à especificação de formato dos países correspondentes;
## • Todos os registos têm uma data de saída superior à data de entrada;
## • Em caso de qualquer erro das condições anteriores, dá so_error S2.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S2.1.



if [[ ! -f "paises.txt" ]]; then
	so_error S2.1 "Ficheiro paises.txt inexistente!"
	exit 1
else
	codigo_valido=$( awk -F'###' '{print $1}' paises.txt)
fi

if [[ ! -f "estacionamentos.txt" ]]; then
	so_success S2.1
	so_success S2.2
	exit 0
fi

if [[ ! -s "estacionamentos.txt" ]]; then
	so_success S2.1 
	so_success S2.2
	exit 0
fi

IFS=$'\n' ##utilizo para garantir que o for lê a linha toda

for linha in $( cat "estacionamentos.txt" ); do

	data_entrada=$( echo "$linha" | awk -F':' '{print $5}' )
	dia_entrada=$( echo "$data_entrada" | cut -d'-' -f3 | cut -c1-2 )
	argumentos_linha=$( echo "$linha" | awk -F':' '{print NF}' )
	
	if [[ "$argumentos_linha" -eq 6 ]]; then
		
		data_saida=$( echo "$linha" | awk -F':' '{print $6}' )
		data_entrada_separada=$(echo "$data_entrada" | sed 's/T/ /; s/h/:/')
        data_saida_separada=$(echo "$data_saida" | sed 's/T/ /; s/h/:/')

        minutos_entrada=$(date -d "$data_entrada_separada" +%s)
        minutos_saida=$(date -d "$data_saida_separada" +%s)
		
		if [[ "$minutos_entrada" -eq "$minutos_saida" ]]; then
			
			so_error S2.1 "Data de entrada igual à data de saída!"
			exit 1
			
		elif [[ "$minutos_entrada" -gt "$minutos_saida" ]]; then
			
			so_error S2.1 "Data de saída inferior à data de entrada!"
			exit 1
		
		fi

	fi
	
	if [[ -n "$(echo "$linha" | awk -F':' '{print $2}')" ]]; then
			
		codigo_pais=$( echo "$linha" | awk -F':' '{print $2}' )
		matricula_estacionamento=$( echo "$linha" | awk -F':' '{print $1}' )
		regra_matricula_saida=$( grep "$codigo_pais" paises.txt | awk -F'###' '{print $3}' )

		if ! echo "$codigo_valido" | grep -q "$codigo_pais"; then	
			
			so_error S2.1 "Código de país inexistente!"
			exit 1

		fi
			
		if  [[ ! "$matricula_estacionamento" =~ $regra_matricula_saida ]]; then
				
			so_error S2.1 "Matrícula não corresponde ao respetivo código de país!"
			exit 1
		
		fi
	fi
done

so_success S2.1

## S2.2. Processamento:
## • O script move, do ficheiro estacionamentos.txt, todos os registos que estejam completos (com registo de entrada e registo de saída), mantendo o formato do ficheiro original, para ficheiros separados com o nome arquivo-<Ano>-<Mês>.park, com todos os registos agrupados pelo ano e mês indicados pelo nome do ficheiro. Ou seja, os registos são removidos do ficheiro estacionamentos.txt e acrescentados ao correspondente ficheiro arquivo-<Ano>-<Mês>.park, sendo que o ano e mês em questão são os do campo <DataSaída>. 
## • Quando acrescentar o registo ao ficheiro arquivo-<Ano>-<Mês>.park, este script acrescenta um campo <TempoParkMinutos> no final do registo, que corresponde ao tempo, em minutos, que durou esse registo de estacionamento (correspondente à diferença em minutos entre os dois campos anteriores).
## • Em caso de qualquer erro das condições anteriores, dá so_error S2.2 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S2.2.
## • O registo em cada ficheiro arquivo-<Ano>-<Mês>.park, tem então o formato:
## <Matrícula:string>:<Código País:string>:<Categoria:char>:<Nome do Condutor:string>: <DataEntrada:AAAA-MM-DDTHHhmm>:<DataSaída:AAAA-MM-DDTHHhmm>:<TempoParkMinutos:int>
## • Exemplo de um ficheiro arquivo-<Ano>-<Mês>.park, para janeiro de 2025:
## 00-00-00:PT:A:José Silva:2025-01-01T12h00:2025-01-01T12h30:30

ficheiro_temporario=$(mktemp)

IFS=$'\n'
for linha in $( cat "estacionamentos.txt" ); do
		
	argumentos_linha=$(echo "$linha" | awk -F':' '{print NF}')

	if [[ ! "$argumentos_linha" -eq 6 ]]; then
		echo "$linha" >> "$ficheiro_temporario"

	else
		matricula=$( echo "$linha" | awk -F':' '{print $1}' )
		codigo_pais=$( echo "$linha" | awk -F':' '{print $2}' )
		categoria=$( echo "$linha" | awk -F':' '{print $3}' )
		nome_condutor=$( echo "$linha" | awk -F':' '{print $4}' )
		data_entrada=$( echo "$linha" | awk -F':' '{print $5}' )
		data_saida=$( echo "$linha" | awk -F':' '{print $6}' )

		data_entrada_separada=$(echo "$data_entrada" | sed  's/T/ /; s/h/:/')
        data_saida_separada=$(echo "$data_saida" | sed  's/T/ /; s/h/:/')
		minutos_entrada=$( date -d "$data_entrada_separada" +%s )
		minutos_saida=$( date -d "$data_saida_separada" +%s )
		tempo_park_minutos=$(( ($minutos_saida - $minutos_entrada)  / 60 ))

		if [[ $tempo_park_minutos -lt 0 ]]; then
			so_error S2.2 "tempo de saída é menor que tempo de entrada"
			exit 1
		fi

		ano=$( echo "$data_saida" | cut -d'-' -f1 )
		mes=$( echo "$data_saida" | cut -d'-' -f2 )

		arquivo="arquivo-$ano-$mes.park"
		
		if ! echo "$matricula:$codigo_pais:$categoria:$nome_condutor:$data_entrada:$data_saida:$tempo_park_minutos" >> "$arquivo"; then
			so_error S2.2 "Falha ao escrever no arquivo"
			exit 1
		fi
	fi

done

mv "$ficheiro_temporario" "estacionamentos.txt"

so_success S2.2
