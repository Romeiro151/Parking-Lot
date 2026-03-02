#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº: 129838      Nome: José Miguel Oliveira Romeiro
## Nome do Módulo: S1. Script: regista_passagem.sh
## Descrição/Explicação do Módulo:
##	
##  Neste módulo, procura-se verificar se dados de entrada num estancionamento cumprem certas regras.
## 	Procura-se inicialmente verificar se os dados de entrada ou saída possuem categoria 
##	(categoria só se verifica na entrada) e código de país válidos e de seguida verificar se a matricula
##  de entrada do veiculo corresponde ou não à regra de maticula do respetivo código de pais. 
##	Por fim, verifica se o nome do condutor que quer entrar está registado no ficheiro _etc_passwd.
##  Para verificar as condições mencionadas a cima usei, maioritáriamente, estruturas if com variáveis
## 	com grep's e awk's.
##  Depois verifica-se se esse veiculo de entrada já está no estacionamento. Se o veículo já está registado
##	como entrada dá erro. Se estiver como saída ou não estiver registado, cria-se um novo registo de entrada.
## 	Caso seja uma saída verifica-se se o veículo já tem um registo de saída criado. Se tiver dá erro, se tiver
## 	apenas um registo de entrada, cria-se um de saída.
## 	Para terminar, cria-se um ficheiro que ordena os registos no ficheiro estacionamentos.txt por hora.
##   
#####################################################################################

## Este script é invocado quando uma viatura entra/sai do estacionamento Park-IUL. Este script recebe todos os dados por argumento, na chamada da linha de comandos, incluindo os <Matrícula:string>, <Código País:string>, <Categoria:char> e <Nome do Condutor:string>.

## S1.1. Valida os argumentos passados e os seus formatos:
## • Valida se os argumentos passados são em número suficiente (para os dois casos exemplificados), assim como se a formatação de cada argumento corresponde à especificação indicada. O argumento <Categoria> pode ter valores: L (correspondente a Ligeiros), P (correspondente a Pesados) ou M (correspondente a Motociclos);
## • A partir da indicação do argumento <Código País>, valida se o argumento <Matrícula> passada cumpre a especificação da correspondente <Regra Validação Matrícula>;
## • Valida se o argumento <Nome do Condutor> é o “primeiro + último” nomes de um utilizador atual do Tigre;
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.1.

if [[ "$#" -eq 4 ]]; then
	
	if [[ "$3" != "L" && "$3" != "P" && "$3" != "M" ]]; then
		so_error S1.1 "Categoria inválida!".
		exit 1 
	fi
	
	if [[ "$2" != "PT" && "$2" != "ES" && "$2" != "FR" && "$2" != "UK" ]]; then
		so_error S1.1 "Código de País inválido"
		exit 1
	fi

	regra_matricula_entrada=$( grep -E "^$2###" paises.txt | awk -F'###' '{print $3}' )

	if [[ ! "$1" =~ $regra_matricula_entrada ]]; then
		so_error S1.1 "Matrícula não corresponde ao país!"
		exit 1
	fi

	primeiro_nome=$( echo "$4" | awk '{print $1}' )
	ultimo_nome=$( echo "$4" | awk '{print $NF}' )
	nomes=$( cut -d':' -f5 _etc_passwd | sed 's/,*$//g')
	contador_nomes=$( echo "$4" | wc -w )

	if [[ $contador_nomes -ne 2 ]]; then
		so_error S1.1 "Número de nomes inválido!"
		exit 1
	fi

	if  ! echo "$nomes" |  grep -q "^$primeiro_nome.*$ultimo_nome"; then
    	so_error S1.1 "O nome do condutor não corresponde a nenhum dos nomes que está a utilizar o tigre"
		exit 1
	fi

so_success S1.1


elif [[ "$#" -eq 1 ]]; then
	
	entrada="$1"	

	if (( $(echo $entrada | tr -cd '/' | wc -c) != 1 )); then
		so_error S1.1 "Número de barras inválido!"
		exit 1
	fi

	codigo_pais=$( echo $entrada | cut -d'/' -f1 )
	matricula=$( echo $entrada | cut -d'/' -f2 )
		
	if [[ $codigo_pais != "PT" && $codigo_pais != "ES" && $codigo_pais != "FR" && $codigo_pais != "UK" ]]; then
		so_error S1.1 "Código de país inválido!"
		exit 1
	fi

	regra_matricula_saida=$( grep $codigo_pais "paises.txt" | awk -F'###' '{print $3}' )
		
	if [[ ! $matricula =~ $regra_matricula_saida ]]; then
		so_error S1.1 "Matrícula não corresponde ao determinado país!"
		exit 1
	fi

	so_success S1.1
	

else
	so_error S1.1 "Número de argumentos inváilido!"
	exit 1
fi


## S1.2. Valida os dados passados por argumento para o script com o estado da base de dados de estacionamentos especificada no ficheiro estacionamentos.txt:
## • Valida se, no caso de a invocação do script corresponder a uma entrada no parque de estacionamento, se ainda não existe nenhum registo desta viatura na base de dados;
## • Valida se, no caso de a invocação do script corresponder a uma saída do parque de estacionamento, se existe um registo desta viatura na base de dados;
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.2 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.2.

if [ "$#" -eq 4 ]; then
	
	if [[ ! "$1" =~ $regra_matricula_saida ]]; then
		so_error S1.1 "Matrícula não corresponde ao país!"
		exit 1
	fi
	
 	matriucla_sem_tracado=$( echo "$1" | sed 's/[- ]//g' )

	if grep -q $matriucla_sem_tracado estacionamentos.txt; then
		
		linha=$( grep $matriucla_sem_tracado estacionamentos.txt | tail -n 1)
		argumentos=$( echo $linha | awk -F':' '{print NF}' )
		
		if [[ $argumentos -eq 5 ]]; then
			so_error S1.2 "Esta matricula já está nesta garagem!"
			exit 1
		
		elif [[ $argumentos -eq 6 ]]; then
			so_success S1.2 
		
		else
			so_error S1.2 "Número de argumentos inválido!"
			exit 1
		fi
	else
		so_success S1.2
	fi

elif [ "$#" -eq 1 ]; then

	matricula_saida_sem_tracado=$( echo "$1" | cut -d'/' -f2 | sed 's/[- ]//g' )
	
	if  grep -q "$matricula_saida_sem_tracado" estacionamentos.txt; then
		linha=$( grep "$matricula_saida_sem_tracado" estacionamentos.txt | tail -n 1)
		argumentos=$( echo $linha | awk -F':' '{print NF}' )
		
		if [[ $argumentos -eq 5 ]]; then
			so_success S1.2

		elif [[ $argumentos -eq 6 ]]; then
			so_error S1.2 "Esta matricula já saiu desta garagem!"
			exit 1
		else
			so_error S1.2 "Número de argumentos inválido!"
			exit 1
		fi
	
	else 
		so_error S1.2 "Não há registos desta matricula na garagem!"
		exit 1
	fi

else
	so_error S1.2 "Número de argumentos inválido!"
	exit 1
fi



## S1.3. Atualiza a base de dados de estacionamentos especificada no ficheiro estacionamentos.txt:
## • Remova do argumento <Matrícula> passado todos os separadores (todos os caracteres que não sejam letras ou números) eventualmente especificados;
## • Especifique como data registada (de entrada ou de saída, conforme o caso) a data e hora do sistema Tigre;
## • No caso de um registo de entrada, crie um novo registo desta viatura na base de dados;
## • No caso de um registo de saída, atualize o registo desta viatura na base de dados, registando a data de saída;
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.3 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.3.
	
data=$(date '+%Y-%m-%dT%Hh%M')

if [ "$#" -eq 4 ]; then

    matriucla_sem_tracado=$(echo "$1" | sed 's/[- ]//g' )
    
	echo "$matriucla_sem_tracado:$2:$3:$4:$data" >> estacionamentos.txt
    
	if [ $? -eq 0 ]; then ##verifico se a operação a cima foi bem realizada (caso seja igual a 0 é sucesso, caso contrário não foi bem sucedida)
        so_success S1.3
    else
        so_error S1.3 "Erro ao criar o registo de entrada"
        exit 1
    fi

elif [ "$#" -eq 1 ]; then

    matriucla_sem_tracado_saida=$(echo "$1" | cut -d'/' -f2 | sed 's/[- ]//g')
	linha=$( grep "$matricula_saida_sem_tracado" estacionamentos.txt | tail -n 1)
	argumentos=$( echo $linha | awk -F':' '{print NF}' )

	if grep -q "$matriucla_sem_tracado_saida" estacionamentos.txt; then
		
		if [[ $argumentos -eq 5 ]]; then
    		
			sed -i "/^$linha\$/ s/$/:$data/" estacionamentos.txt

			if [ $? -eq 0 ]; then
       			 so_success S1.3
    		else
        		so_error S1.3 "Erro ao atualizar o registo de saída"
        		exit 1
    		fi
		else
			so_error S1.3 "O carro já saiu da garagem!"
			exit 1
		fi
	else
		so_error S1.3 "Não há qualquer registo dessa matricula no estacionamento"
	fi
else
    so_error S1.3 "Número de argumentos inválido!"
    exit 1
fi

## S1.4. Lista todos os estacionamentos registados, mas ordenados por saldo:
## • O script deve criar um ficheiro chamado estacionamentos-ordenados-hora.txt igual ao que está no ficheiro estacionamentos.txt, com a mesma formatação, mas com os registos ordenados por ordem crescente da hora (e não da data) de entrada das viaturas.
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.4 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.4.

sort -t':' -k5.12,5.13 -k5.15,5.16 estacionamentos.txt > estacionamentos-ordenados-hora.txt

so_success S1.4



