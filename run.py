import argparse
import subprocess
import time


run_cases = frozenset({"4-5", "6-7", "8", "9-10"})


def build_with_cmake():
    subprocess.run(["cmake", "-S", ".", "-B", "build"])
    subprocess.run(["cmake", "--build", "build", "--target", "all"])


def run_server(folder: str, port: int):
    assert folder in run_cases

    with (open("server.out", "w") as so, open("server.err", "w") as se):
        return subprocess.Popen([f"build/{folder}/server", str(port)], stdout=so, stderr=se)


def run_gardener(folder: str, port: int, n: int):
    assert folder in run_cases

    with (open(f"gardener{n}.out", "w") as so, open(f"gardener{n}.err", "w") as se):
        return subprocess.Popen([f"build/{folder}/gardener", "127.0.0.1", str(port)], stdout=so, stderr=se)


def run_flowerbed(folder: str, port: int):
    assert folder in run_cases

    with (open("flowerbed.out", "w") as so, open("flowerbed.err", "w") as se):
        return subprocess.Popen([f"build/{folder}/flowerbed", "127.0.0.1", str(port)], stdout=so, stderr=se)


def run_monitor(folder: str, port: int):
    assert folder in run_cases
    with (open("monitor.out", "w") as mo, open("monitor.err", "w") as me):
        return subprocess.Popen([f"build/{folder}/monitor", "127.0.0.1", str(port)], stdout=mo, stderr=me)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run_case", help="Папка с программами (соответствует оценке, на которую выполнена программа)", type=str, choices=run_cases)
    parser.add_argument("--port", "-p", help="Порт, на котором запустится сервер", type=int, default=12345)

    args = parser.parse_args()

    build_with_cmake()

    processes = []

    match args.run_case:
        case "4-5":
            print("Launching server")
            processes.append(run_server("4-5", args.port))
            time.sleep(1)
            print("Launching gardeners")
            processes.append(run_gardener("4-5", args.port, 1))
            processes.append(run_gardener("4-5", args.port, 2))
            time.sleep(1)
            print("Flowerbed")
            processes.append(run_flowerbed("4-5", args.port))

        case "6-7" | "8" | "9-10":
            print("Launching server")
            processes.append(run_server(args.run_case, args.port))
            time.sleep(1)
            print("Launching monitor")
            processes.append(run_monitor(args.run_case, args.port))
            print("Launching gardeners")
            processes.append(run_gardener(args.run_case, args.port, 1))
            processes.append(run_gardener(args.run_case, args.port, 2))
            time.sleep(1)
            print("Flowerbed")
            processes.append(run_flowerbed(args.run_case, args.port))
        case _:
            raise ValueError(f"Unknown run case {args.run_case}")

    time.sleep(40)
    for p in processes:
        p.terminate()

if __name__ == "__main__":
    main()
