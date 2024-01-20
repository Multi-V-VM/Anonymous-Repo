import pickle
import common_util
from multiprocessing import Pool

cmd = ["bt", "cg", "ep", "ft", "lu", "mg", "sp"]
arg = [[], [], [], [], [], [], []]


pool = Pool(processes=5)

# run the benchmarks
results = []
for i in range(len(cmd)):
    for j in range(len(common_util.aot_variant)):
        for env in common_util.list_of_arg:
            aot = cmd[i] + common_util.aot_variant[j]
            results.append(pool.apply_async(common_util.run, (aot, arg[i], env)))
pool.close()
pool.join()

# print the results
results = [x.get() for x in results]
# serialize the results
with open("bench_nas_results.pickle", "wb") as f:
    pickle.dump(results, f)
for exec, output in results:
    print(exec)
    lines = output.split("\n")
    for line in lines:
        if line.__contains__("Execution time:"):
            print(line)

# read the results
# with open("bench_results.pickle", "rb") as f:
#     results = pickle.load(f)
