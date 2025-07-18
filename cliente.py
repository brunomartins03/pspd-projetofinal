import socket
import json

HOST = "192.168.49.2" # Ajustar conforme necessário
PORT = 30000

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    try:
        sock.connect((HOST, PORT))
    except Exception as e:
        print(f"Não foi possível conectar ao servidor em {HOST}:{PORT}. Erro: {e}")
        exit(1)
    
    print(f"\nConectado ao host em: {HOST}:{PORT}\n")

    print("#==================================================#\n")

    try:
        while True:
            engine = ""
            while engine not in ["mpi", "spark"]:
                engine = input("Escolha a Engine (mpi/spark): ")
                if engine not in ["mpi", "spark"]:
                    print(f"Opção \"{engine}\" inválida. Tente novamente.\n")
                    continue

            if engine.lower() == "sair":
                break
            else :
                print()
            
            powmin = powmax = ""
            while powmin == "" or powmax == "":
                powmin = input("POWMIN: ")
                powmax = input("POWMAX: ")

                if not powmin.isdigit() or not powmax.isdigit():
                    print("POWMIN e POWMAX devem ser números inteiros. Tente novamente.\n")
                    powmin = powmax = ""
                    continue
                elif int(powmin) < 0 or int(powmax) < 0:
                    print("POWMIN e POWMAX devem ser números inteiros positivos. Tente novamente.\n")
                    powmin = powmax = ""
                    continue
                elif int(powmin) > int(powmax):
                    print(f"POWMIN ({powmin}) não pode ser maior que POWMAX ({powmax}). Tente novamente.\n")
                    powmin = powmax = ""
                    continue

            print("\n||================================================||\n")


            payload = {
                "engine": engine,
                "powmin": int(powmin),
                "powmax": int(powmax)
            }

            sock.sendall(json.dumps(payload).encode())
            response = sock.recv(4096)
            response_data = json.loads(response.decode())

            print(f"Status: {response_data.get('status', 'Erro')}")
            print(f"Resposta do servidor:\n\n{response_data.get('data', 'Sem resultado')}")
            print("#==================================================#\n")
    except KeyboardInterrupt:
        print("\n\n||================================================||\n")
        print("Saindo.")
    except Exception as e:
        print("\n||************************************************||\n")
        print(f"Erro: {e}")
    finally:
        sock.close()
        print("Conexão fechada.")
    print("#==================================================#")

