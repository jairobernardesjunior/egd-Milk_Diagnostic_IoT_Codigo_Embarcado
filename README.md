Essa parte do projeto de hardware e software embutido Milk_Diagnostic_Hardware visa desenvolver o software base que vai fazer
as funções de captura da temperatura do leite, da temperatura ambiente, da data e hora do satélite, localização onde o aparelho
se encontra e armazenar esses dados no sdcard. Se a temperatura do leite exceder do limite superior, um sms de alerta será 
enviado para o nro do phone cadastrado no arquivo de parâmetros.
Milk Diagnostic Hardware é um dispositivo IoT que vai medir a temperatura do leite no reservatório da fazenda e enviar os dados
via sms 24 horas por dia e 7 dias da semana, onde será recebido na caixa de mensagem do gmail receptor, sendo o valor dos índices, capturados por uma aplicação python, rodando em AWS Lambda, transformando os dados e armazenando em AWS BucketS3 como arquivo json, para análise do cientista e analista de dados.