#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocolo.h"
#include "jogo.h"

int ler_palavra_com_timeout(char *palavra, int tamanho, int segundos);

int main(void)
{
    int sock;
    struct sockaddr_in servidor;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("Erro ao criar socket");
        return EXIT_FAILURE;
    }
    printf("Socket do cliente criado!\n");

    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(PORTA_PADRAO);
    if (inet_pton(AF_INET, "127.0.0.1", &servidor.sin_addr) <= 0) {
        perror("Endereco IP invalido");
        close(sock);
        return EXIT_FAILURE;
    }

    if (connect(sock, (struct sockaddr *)&servidor, sizeof(servidor)) < 0) {
        perror("Erro ao conectar");
        close(sock);
        return EXIT_FAILURE;
    }
    printf("Conectado ao servidor!\n");

    char mensagem[BUFFER_SIZE];

    while (1) {
        int resultado = receber_mensagem(sock, mensagem, sizeof(mensagem));

        if (resultado <= 0) {
            printf("Conexao encerrada.\n");
            break;
        }

        char *tipo = strtok(mensagem, "|\n");

        if (tipo == NULL) continue;
        
        char resposta[BUFFER_SIZE];

        if (strcmp(tipo, PROTO_NOME) == 0) {
            printf("╔══════════════════════════════════════╗\n║     BATALHA DE PALAVRAS — Cliente    ║\n╚══════════════════════════════════════╝\nConectando a 127.0.0.1:7070...\nConectado!\n\n");

            char nome[NOME_SIZE];

            printf("Digite seu nome: ");
            fflush(stdout);

            if (fgets(nome, sizeof(nome), stdin) == NULL) {
                printf("Erro ao ler nome.\n");
                break;
            }

            nome[strcspn(nome, "\n")] = '\0';

            snprintf(resposta, sizeof(resposta), "%s%s%s\n", PROTO_NOME, PROTO_SEP, nome);

            enviar_mensagem(sock, resposta);

            printf("Bem-vindo, %s!\n\n", nome);
        } 
        else if (strcmp(tipo, PROTO_AGUARDE) == 0) {
            printf("Conectado! Aguardando outro jogador para iniciar...\n\n");
        }
        else if (strcmp(tipo, PROTO_MSG) == 0) {
            char *msg = strtok(NULL, "|\n");
            printf("Batalha de Palavras! %s — %d rodadas. Boa sorte!\n\n", msg, TOTAL_RODADAS);
        }
        else if (strcmp(tipo, PROTO_RODADA) == 0) {
            char *rodada = strtok(NULL, "|");
            char *letra  = strtok(NULL, "|");
            char *tempo  = strtok(NULL, "|\n");
            
            int tempo_limite = atoi(tempo);

            printf("╔══════════════════════════════════╗\n║        RODADA %s de %d             ║\n║   Letra: [%s]   Tempo: %s seg     ║\n║   Mínimo: %d caracteres           ║\n╚══════════════════════════════════╝\n",
                rodada, TOTAL_RODADAS, letra, tempo, MIN_CARACTERES);

            char palavra[473];

            printf("Sua palavra: ");
            fflush(stdout); 

            int resultado = ler_palavra_com_timeout(palavra, sizeof(palavra), tempo_limite);

            if (resultado < 0) {
                printf("Erro ao ler palavra.\n");
                break;
            }

            if (resultado == 0) {
                printf("\nTempo esgotado!\n");

                tcflush(STDIN_FILENO, TCIFLUSH);

                snprintf(resposta, sizeof(resposta), "%s%s\n", PROTO_TIMEOUT, PROTO_SEP);
            }
            else {
                snprintf(resposta, sizeof(resposta), "%s%s%s\n", PROTO_PALAVRA, PROTO_SEP, palavra);

                printf("Enviado: %s - aguardando resultado...\n", palavra);
            }

            if (enviar_mensagem(sock, resposta) < 0) break;
        }
        else if (strcmp(tipo, PROTO_RESULTADO) == 0) {
            char *resultado = strtok(NULL, "|\n");

            printf("Resultado da rodada: %s\n", resultado);
        } 
        else if (strcmp(tipo, PROTO_PLACAR) == 0) {
            char *nome1 = strtok(NULL, "|");
            char *pontos1  = strtok(NULL, "|");
            char *nome2  = strtok(NULL, "|");
            char *pontos2  = strtok(NULL, "|\n");

            printf("┌─────────────────────────────┐\n│  PLACAR: %s %s x %s %s │\n└─────────────────────────────┘\n\n",
                nome1, pontos1, pontos2, nome2);
        }
        else if (strcmp(tipo, PROTO_FIM) == 0) {
            char *texto = strtok(NULL, "|\n");

            printf("%s\n\n", texto);

            break;
        }
    }

    close(sock);
    return 0;
}

int ler_palavra_com_timeout(char *palavra, int tamanho, int segundos) {
    fd_set leitura;
    FD_ZERO(&leitura);
    FD_SET(STDIN_FILENO, &leitura);

    struct timeval timeout;
    timeout.tv_sec = segundos;
    timeout.tv_usec = 0;

    int resultado = select(STDIN_FILENO + 1, &leitura, NULL, NULL, &timeout);

    if (resultado < 0) {
        perror("Erro no select");
        return -1;
    }

    if (resultado == 0) return 0;

    if (fgets(palavra, tamanho, stdin) == NULL) return -1;

    palavra[strcspn(palavra, "\n")] = '\0';

    return 1;
}