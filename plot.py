import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# Lê os dados do arquivo CSV e limpa os nomes das colunas
df = pd.read_csv("saida2.csv")
df.columns = df.columns.str.strip()  # Remove espaços em branco nos nomes das colunas

# Cálculo da aceleração para cada N
print("Aceleração para cada dimensão N (tempo com 1 thread dividido pelo tempo com múltiplas threads):")
for N in sorted(df["N"].unique()):
    base_time = df[(df["N"] == N) & (df["T"] == 1)]["Dt"].values[0]
    print(f"\nDimensão N = {N}")
    for T in [1, 2, 3, 5, 10, 20]:
        t_time = df[(df["N"] == N) & (df["T"] == T)]["Dt"].values[0]
        speedup = base_time / t_time
        print(f"  Threads = {T}: Tempo de Execução = {t_time:.7f} segundos | Aceleração = {speedup:.2f}")


# Geração dos gráficos de barras empilhadas para cada dimensão
for N in sorted(df["N"].unique()):
    df_N = df[df["N"] == N].sort_values("T")
    
    threads = df_N["T"].values
    Dc = df_N["Dc"].values
    Dp = df_N["Dp"].values
    Dg = df_N["Dg"].values

    bar_width = 0.6
    x = np.arange(len(threads))

    plt.figure(figsize=(8, 5))
    plt.bar(x, Dc, width=bar_width, label="Carregamento", color="skyblue")
    plt.bar(x, Dp, bottom=Dc, width=bar_width, label="Execução", color="orange")
    plt.bar(x, Dg, bottom=Dc + Dp, width=bar_width, label="Desalocação", color="lightgreen")

    plt.xticks(x, threads)
    plt.xlabel("Número de Threads")
    plt.ylabel("Tempo Médio (s)")
    plt.title(f"Medidas de tempos para dimensão = {N}")
    plt.legend()
    plt.tight_layout()
    plt.show()
