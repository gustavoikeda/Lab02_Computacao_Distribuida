#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "protocolo.h"
#include "jogo.h"

void *thread_partida(void *arg);

int main(void)
{
    int servidor_fd;
    struct sockaddr_in endereco;

    servidor_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (servidor_fd < 0) {
        perror("Erro ao criar socket");
        return EXIT_FAILURE;
    }
    printf("Socket criado com sucesso!\n");
        
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(PORTA_PADRAO);

    if (bind(servidor_fd, (struct sockaddr *)&endereco, sizeof(endereco)) < 0) {
        perror("Erro no bind");
        close(servidor_fd);
        return EXIT_FAILURE;
    }

    if (listen(servidor_fd, 5) < 0) {
        perror("Erro no listen");
        close(servidor_fd);
        return EXIT_FAILURE;
    }
    printf("Servidor aguardando conexoes na porta %d...\n", PORTA_PADRAO);
        
    srand(time(NULL));
    
    while (1) {
        Partida *partida = malloc(sizeof(Partida));

        if (partida == NULL) {
            perror("Erro no malloc");
            continue;
        }

        Jogador jogador1 = {0};
        Jogador jogador2 = {0};

        jogador1.socket_fd = accept(servidor_fd, NULL, NULL);

        if (jogador1.socket_fd < 0) {
            perror("Erro no accept");
            close(servidor_fd);
            return EXIT_FAILURE;
        }
        printf("Cliente 1 conectado!\n");

        if (solicitar_nome(&jogador1) < 0) {
            printf("Erro ao obter nome do jogador 1.\n");
            close(jogador1.socket_fd);
            close(servidor_fd);
            return EXIT_FAILURE;
        }
        printf("Jogador 1: %s\n", jogador1.nome);

        char mensagem[BUFFER_SIZE];

        snprintf(mensagem, sizeof(mensagem), "%s%sEsperando outro jogador...\n", PROTO_AGUARDE, PROTO_SEP);

        enviar_mensagem(jogador1.socket_fd, mensagem);


        //CLIENTE 2:
        jogador2.socket_fd = accept(servidor_fd, NULL, NULL);

        if (jogador2.socket_fd < 0) {
            perror("Erro no accept");
            close(jogador1.socket_fd);
            close(servidor_fd);
            return EXIT_FAILURE;
        }
        printf("Cliente 2 conectado!\n");

        if (solicitar_nome(&jogador2) < 0) {
            printf("Erro ao obter nome do jogador 2.\n");
            close(jogador2.socket_fd);
            close(jogador1.socket_fd);
            close(servidor_fd);
            return EXIT_FAILURE;
        }
        printf("Jogador 2: %s\n", jogador2.nome);

        snprintf(mensagem, sizeof(mensagem), "%s%s%s vs %s\n", PROTO_MSG, PROTO_SEP, jogador1.nome, jogador2.nome);

        enviar_mensagem(jogador1.socket_fd, mensagem);
        enviar_mensagem(jogador2.socket_fd, mensagem);

        partida->jogador1 = jogador1;
        partida->jogador2 = jogador2;

        pthread_t thread;

        if (pthread_create(&thread, NULL, thread_partida, partida) != 0) {
            perror("Erro ao criar thread");

            close(partida->jogador1.socket_fd);
            close(partida->jogador2.socket_fd);
            free(partida);

            continue;
        }

        pthread_detach(thread);
    }
    
    close(servidor_fd);

    return 0;
}

void *thread_partida(void *arg) {
    Partida *partida = (Partida *)arg;

    comecar_partida(&partida->jogador1, &partida->jogador2);

    close(partida->jogador1.socket_fd);
    close(partida->jogador2.socket_fd);

    free(partida);

    return NULL;
}