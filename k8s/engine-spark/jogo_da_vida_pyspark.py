import time
from pyspark.sql import SparkSession

def uma_vida(tabul_in, tabul_out, tam):
    def vizinhos_vivos(cell, neighbors):
        vizviv = sum(neighbors)
        if cell == 1 and vizviv < 2:
            return 0
        elif cell == 1 and vizviv > 3:
            return 0
        elif cell == 0 and vizviv == 3:
            return 1
        return cell

    # Aplicando a regra do Jogo da Vida
    for i in range(1, tam + 1):
        for j in range(1, tam + 1):
            vizinhos = [
                tabul_in[i - 1][j - 1], tabul_in[i - 1][j], tabul_in[i - 1][j + 1],
                tabul_in[i][j - 1], tabul_in[i][j + 1],
                tabul_in[i + 1][j - 1], tabul_in[i + 1][j], tabul_in[i + 1][j + 1]
            ]
            tabul_out[i][j] = vizinhos_vivos(tabul_in[i][j], vizinhos)

def calcula_cenario(pow_min, pow_max):
    # Inicialize a SparkSession
    spark = SparkSession.builder.appName("JogoDaVidaPySpark").getOrCreate()

    # Função que inicializa o tabuleiro com um padrão específico
    def init_tabul(tam):
        tabul_in = [[0] * (tam + 2) for _ in range(tam + 2)]
        tabul_out = [[0] * (tam + 2) for _ in range(tam + 2)]
        tabul_in[1][2] = 1
        tabul_in[2][3] = 1
        tabul_in[3][1] = 1
        tabul_in[3][2] = 1
        tabul_in[3][3] = 1
        return tabul_in, tabul_out

    # Função que valida o tabuleiro final
    def correto(tabul_in, tam):
        return (
            tabul_in[tam - 2][tam - 1] == 1 and
            tabul_in[tam - 1][tam] == 1 and
            tabul_in[tam][tam - 2] == 1 and
            tabul_in[tam][tam - 1] == 1 and
            tabul_in[tam][tam] == 1
        )

    for pow in range(pow_min, pow_max + 1):
        tam = 1 << pow
        tabul_in, tabul_out = init_tabul(tam)
        
        # Medindo o tempo de execução
        start_time = time.time()

        # Simula as gerações do Jogo da Vida
        for _ in range(2 * (tam - 3)):
            uma_vida(tabul_in, tabul_out, tam)
            uma_vida(tabul_out, tabul_in, tam)

        end_time = time.time()
        elapsed_time = end_time - start_time

        # Validação e saída
        if correto(tabul_in, tam):
            print(f"**RESULTADO CORRETO para tam={tam}**")
        else:
            print(f"**RESULTADO INCORRETO para tam={tam}**")
        
        print(f"Tempo de execução para tam={tam}: {elapsed_time:.4f} segundos")

    # Finalize a SparkSession
    spark.stop()

def main(pow_min, pow_max):
    print(f"Recebendo valores POWMIN={pow_min} e POWMAX={pow_max}")
    calcula_cenario(pow_min, pow_max)
