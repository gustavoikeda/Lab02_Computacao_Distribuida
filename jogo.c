/* Alunos
Gustavo Kiyoshi Ikeda RA: 10439179
Pedro Montarroyos de Pinho RA: 10440213
Felipe Marques Leite Martha RA: 10437877
*/
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#include <sys/select.h>
#include <time.h>

#include "jogo.h"

int enviar_mensagem(int socket_fd, const char *mensagem) {
    ssize_t enviados;

    enviados = send(socket_fd, mensagem, strlen(mensagem), 0);

    if (enviados < 0) {
        perror("Erro no send");
        return -1;
    }

    return 0;
}

int receber_mensagem(int socket_fd, char *buffer, int tamanho) {
    int posicao = 0;
    char caractere;

    while (posicao < tamanho - 1) {
        ssize_t resultado = recv(socket_fd, &caractere, 1, 0);

        if (resultado < 0) {
            perror("Erro no recv");
            return -1;
        }

        if (resultado == 0) return 0;

        buffer[posicao] = caractere;
        posicao++;

        if (caractere == '\n') break;
    }

    buffer[posicao] = '\0';

    return posicao;
}

int solicitar_nome(Jogador *jogador) {
    char buffer[BUFFER_SIZE];
    char mensagem[BUFFER_SIZE];

    snprintf(mensagem, sizeof(mensagem), "%s%s\n", PROTO_NOME, PROTO_SEP);

    if (enviar_mensagem(jogador->socket_fd, mensagem) < 0) return -1;

    int resultado = receber_mensagem(jogador->socket_fd, buffer, sizeof(buffer));

    if (resultado <= 0) return -1;

    char *tipo;
    char *nome;

    tipo = strtok(buffer, "|");
    nome = strtok(NULL, "|\n");

    if (tipo == NULL || nome == NULL) return -1;

    if (strcmp(tipo, PROTO_NOME) != 0) return -1;

    strncpy(jogador->nome, nome, NOME_SIZE - 1);
    jogador->nome[NOME_SIZE - 1] = '\0';

    return 0;
}

char gerar_letra(void) {
    return 'A' + (rand() % 26);
}

int comecar_partida(Jogador *jogador1, Jogador *jogador2) {
    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    char mensagem[BUFFER_SIZE];
        
    for (int i = 1; i <= TOTAL_RODADAS; i++) {
        char letra = gerar_letra();

        snprintf(mensagem, sizeof(mensagem), "%s%s%d%s%c%s%d\n", PROTO_RODADA, PROTO_SEP, i, PROTO_SEP, letra, PROTO_SEP, TEMPO_LIMITE);

        if (enviar_mensagem(jogador1->socket_fd, mensagem) < 0) return -1;
        if (enviar_mensagem(jogador2->socket_fd, mensagem) < 0) return -1;

        int respondeu1;
        int respondeu2;

        if (receber_respostas(jogador1, jogador2, buffer1, buffer2, &respondeu1, &respondeu2) < 0) return -1;

        char *palavra_jogador1 = NULL;
        char *palavra_jogador2 = NULL;

        int pontuacao1 = 0;
        int pontuacao2 = 0;

        if (respondeu1) {
            char *tipo1 = strtok(buffer1, "|");

            if (tipo1 != NULL && strcmp(tipo1, PROTO_PALAVRA) == 0) {
                palavra_jogador1 = strtok(NULL, "|\n");

                if (palavra_jogador1 != NULL) pontuacao1 = validar_palavra(palavra_jogador1, letra);
            }
            else if (tipo1 != NULL && strcmp(tipo1, PROTO_TIMEOUT) == 0) pontuacao1 = 0;
        }

        if (respondeu2) {
            char *tipo2 = strtok(buffer2, "|");

            if (tipo2 != NULL && strcmp(tipo2, PROTO_PALAVRA) == 0) {
                palavra_jogador2 = strtok(NULL, "|\n");

                if (palavra_jogador2 != NULL) pontuacao2 = validar_palavra(palavra_jogador2, letra);
            }
            else if (tipo2 != NULL && strcmp(tipo2, PROTO_TIMEOUT) == 0) pontuacao2 = 0;
            
        }
        if (palavra_jogador1 != NULL && palavra_jogador2 != NULL && strcasecmp(palavra_jogador1, palavra_jogador2) == 0) {
            pontuacao1 = 0;
            pontuacao2 = 0;
        }


        printf(
            "Rodada %d | %s: respondeu=%d palavra=%s pontos=%d\n",
            i,
            jogador1->nome,
            respondeu1,
            palavra_jogador1 != NULL ? palavra_jogador1 : "NULL",
            pontuacao1
        );

        printf(
            "Rodada %d | %s: respondeu=%d palavra=%s pontos=%d\n",
            i,
            jogador2->nome,
            respondeu2,
            palavra_jogador2 != NULL ? palavra_jogador2 : "NULL",
            pontuacao2
        );

        
        snprintf(mensagem, sizeof(mensagem), "%s%s+%d\n", PROTO_RESULTADO, PROTO_SEP, pontuacao1);
        if (enviar_mensagem(jogador1->socket_fd, mensagem) < 0) return -1;

        snprintf(mensagem, sizeof(mensagem), "%s%s+%d\n", PROTO_RESULTADO, PROTO_SEP, pontuacao2);
        if (enviar_mensagem(jogador2->socket_fd, mensagem) < 0) return -1;

        jogador1->pontuacao += pontuacao1;
        jogador2->pontuacao += pontuacao2;

        snprintf(mensagem, sizeof(mensagem), "%s%s%s%s%d%s%s%s%d\n", PROTO_PLACAR, PROTO_SEP, jogador1->nome, PROTO_SEP, jogador1->pontuacao, PROTO_SEP, jogador2->nome, PROTO_SEP, jogador2->pontuacao);
        
        if (enviar_mensagem(jogador1->socket_fd, mensagem) < 0) return -1;
        if (enviar_mensagem(jogador2->socket_fd, mensagem) < 0) return -1;
    }

    if (jogador1->pontuacao == jogador2->pontuacao) {
        snprintf(mensagem, sizeof(mensagem), "%s%s%s!\n", PROTO_FIM, PROTO_SEP, "Empate");
    } else if (jogador1->pontuacao > jogador2->pontuacao) {
        snprintf(mensagem, sizeof(mensagem), "%s%s%s %s!\n", PROTO_FIM, PROTO_SEP, jogador1->nome, "venceu");
    } else {
        snprintf(mensagem, sizeof(mensagem), "%s%s%s %s!\n", PROTO_FIM, PROTO_SEP, jogador2->nome, "venceu");
    }
    
    if (enviar_mensagem(jogador1->socket_fd, mensagem) < 0) return -1;
    if (enviar_mensagem(jogador2->socket_fd, mensagem) < 0) return -1;

    return 0;
}

int validar_palavra(const char *palavra, char letra) {
    if (palavra == NULL) return 0;

    if (strlen(palavra) < MIN_CARACTERES) return 0;

    if (toupper((unsigned char)palavra[0]) != toupper((unsigned char)letra)) return 0;

    for (int i = 0; palavra[i] != '\0'; i++)
        if (!isalpha((unsigned char)palavra[i])) return 0;

    return 1;
}

int receber_respostas(Jogador *jogador1, Jogador *jogador2, char *buffer1, char *buffer2, int *respondeu1, int *respondeu2) {
    *respondeu1 = 0;
    *respondeu2 = 0;

    time_t inicio = time(NULL);

    while (!(*respondeu1) || !(*respondeu2)) {
        int restante = TEMPO_LIMITE - (int)(time(NULL) - inicio);

        if (restante <= 0) break;

        fd_set leitura;
        FD_ZERO(&leitura);

        if (!(*respondeu1)) FD_SET(jogador1->socket_fd, &leitura);
        if (!(*respondeu2)) FD_SET(jogador2->socket_fd, &leitura);

        struct timeval timeout;

        timeout.tv_sec = restante;
        timeout.tv_usec = 0;

        int maior_fd = jogador1->socket_fd > jogador2->socket_fd ? jogador1->socket_fd : jogador2->socket_fd;

        int resultado = select(maior_fd + 1, &leitura, NULL, NULL, &timeout);

        if (resultado < 0) {
            perror("Erro no select");
            return -1;
        }

        if (resultado == 0) break;

        if (!(*respondeu1) && FD_ISSET(jogador1->socket_fd, &leitura)) {
            int resultado1 = receber_mensagem(jogador1->socket_fd, buffer1, BUFFER_SIZE);

            if (resultado1 <= 0) return -1;

            *respondeu1 = 1;
        }

        if (!(*respondeu2) && FD_ISSET(jogador2->socket_fd, &leitura)) {
            int resultado2 = receber_mensagem(jogador2->socket_fd, buffer2, BUFFER_SIZE);

            if (resultado2 <= 0) return -1;

            *respondeu2 = 1;
        }
    }

    return 0;
}
