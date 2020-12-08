from subprocess import check_output
from scipy.stats import gmean
import matplotlib.pyplot as plt

def test_parameters(s, p, v, algo, allo):
    out = check_output(
        "./sortArrays {:d} {:d} {:d} {:s} {:s} 1000000 diskFile.dat".format(
            s, p, v, algo, allo
        ),
        shell=True
    ).decode('utf-8').split('\n')[-6:]
    bubble = 0
    quick = 0
    merge = 0
    for line in [x.strip() for x in out]:
        if "bubble-0" in line:
            bubble = int(line.split()[4])
        if "quick-1" in line:
            quick = int(line.split()[4])
        if "merge-2" in line:
            merge = int(line.split()[4])
    return ([bubble, quick, merge])

PAGE = 6
PHYS = 8
VRTL = 10

results = {
    'b': [],
    'q': [],
    'm': []
}

for s in range(0, PAGE-1):
    p = PHYS - s
    v = VRTL - s
    b = []
    q = []
    m = []
    for pol in ['global', 'local']:
        for algo in ['NRU', 'LRU', 'FIFO', 'SC', 'WSClock']:
            res = test_parameters(s, p, v, algo, pol)
            b.append(res[0])
            q.append(res[1])
            m.append(res[2])
    results['b'].append([s, gmean(b)])
    results['q'].append([s, gmean(q)])
    results['m'].append([s, gmean(m)])
print(results)

plt.clf()
plt.xlabel('Page Size')
plt.ylabel('# of Page Replacements')
plt.plot([x[1] for x in results['b']], label='Bubble')
plt.plot([x[1] for x in results['q']], label='Quick')
plt.plot([x[1] for x in results['m']], label='Merge')
plt.legend()
plt.savefig('./Sorts.png')
