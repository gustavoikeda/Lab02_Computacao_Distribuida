#ifndef JOGO_H
#define JOGO_H

#include "protocolo.h"

typedef struct {
    int socket_fd;
    char nome[NOME_SIZE];
    int pontuacao;
} Jogador;

typedef struct {
    Jogador jogador1;
    Jogador jogador2;
} Partida;

int comecar_partida(Jogador *jogador1, Jogador *jogador2);
int receber_respostas(Jogador *jogador1, Jogador *jogador2, char *buffer1, char *buffer2, int *respondeu1, int *respondeu2);
int enviar_mensagem(int socket_fd, const char *mensagem);
int receber_mensagem(int socket_fd, char *buffer, int tamanho);
int solicitar_nome(Jogador *jogador);
char gerar_letra(void);
int validar_palavra(const char *palavra, char letra);

#endif
