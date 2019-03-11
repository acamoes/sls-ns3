#!bin/bash

#Este script recebe como argumento um ficheiro apenas com as linhas de movimento 
#de um ficheiro de mobilidade .tcl e adiciona um espaçamento de 200. Desta forma os carros
#em vez de estarem a 10metros uns dos outros passam a estar a 210.


#exec: bash script.sh

#author: André Camões

declare -a array

space=' '
a='"$node_(';
b=')'

i='10'
change='6'
# sed -i 's/old-word/new-word/g' *.txt

#for i in {96..99}
#do
#count=200

while read line
do
	#Split por espaco
	array=(${line// / })
	#echo "$a$i$b"
	#echo ${array[3]}
	if [ "$a$i$b" == ${array[3]} ]
	then
	#myvar=$(echo "scale=3; ${array[5]} + $count " | bc)
	#count=$count+200
	#Agora tenho de criar uma string 
	#string=${array[0]}$space${array[1]}$space${array[2]}$space${array[3]}$space${array[4]}$space$myvar$space${array[6]}$space${array[7]}
	#echo -e $string >> output.txt

	sed -i "s/"$a$i$b"/$change/g" input.txt

	#else
	#echo -e $line >> output.txt
	fi
	
done < "input.txt"
done
