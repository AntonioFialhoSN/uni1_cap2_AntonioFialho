# uni1_cap2_AntonioFialho

Projeto desenvolvido para simular diferentes níveis de atividade utilizando um joystick conectado ao Raspberry Pi Pico, com feedback visual por LED RGB e sonoro por um buzzer. O sistema também explora a multitarefa com dois núcleos do microcontrolador.

## 📌 Descrição

O sistema monitora o eixo Y de um joystick analógico e determina o nível de atividade de acordo com a posição:

- **Atividade Baixa:** Joystick puxado para baixo → LED Verde aceso.
- **Atividade Moderada:** Joystick na posição central → LED Azul aceso.
- **Atividade Alta:** Joystick empurrado para cima → LED Vermelho aceso + Buzzer ativado.

A arquitetura multitarefa divide a responsabilidade entre os dois núcleos do RP2040:

- **Core 0:** Leitura do joystick e envio do estado via FIFO.
- **Core 1:** Atualização dos LEDs e buzzer com base nos dados recebidos.

## 🎯 Objetivos

- Compreender a comunicação entre núcleos no RP2040.
- Utilizar o ADC para leitura analógica do joystick.
- Controlar saídas digitais (LEDs, Buzzer).
- Explorar tarefas em paralelo com `pico/multicore.h`.

## 🧰 Hardware Utilizado

- Raspberry Pi Pico
- Joystick analógico (2 eixos)
- LED RGB (com resistores)
- Buzzer ativo
- Cabos jumper e protoboard

## 🧠 Estrutura do Código

- `core0_entry()`: Responsável pela leitura do joystick e definição do estado.
- `core1_entry()`: Aguarda dados na FIFO e atualiza o LED e o buzzer.
- `alarme_callback()`: Envia periodicamente o estado para o Core 1 a cada 2 segundos.
- Utiliza PWM para controle do buzzer.

## 🚀 Como Rodar

1. Instale o [SDK do Raspberry Pi Pico](https://github.com/raspberrypi/pico-sdk). (version=1.51)
2. Clone este repositório:
   ```bash
   git clone https://github.com/AntonioFialhoSN/uni1_cap2_AntonioFialho.git
