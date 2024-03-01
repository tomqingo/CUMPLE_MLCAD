import os
import subprocess

# run command
def run(command):
    print(command)
    os.system('/bin/bash -c \'{0}\''.format(command))

# run modes
modes = ['gdb', 'valgrind', 'vgdb', 'gprof']
mode_prefixes = {
    None        : '',
    'gprof'     : '',
    'gdb'       : 'gdb --args',
    'valgrind'  : 'valgrind',
    'vgdb'      : 'valgrind --vgdb-error=0'
}

# benchmarks
class Benchmark:
    def __init__(self, full_name, abbr_name):
        self.full_name = full_name
        self.abbr_name = abbr_name
    def __repr__(self):
        return self.full_name

class Benchmarks:
    def __init__(self):
        self.__bms = []
        self.__bm_sets = dict()

    def add(self, full_name_pat, abbr_name_pat, ids):
        for id in ids:
            self.__bms.append(Benchmark(full_name_pat.format(id), abbr_name_pat.format(id)))

    def get_bm(self, name):
        # TODO: can be faster by using hash table
        for bm in self.__bms:
            if name == bm.abbr_name or name == bm.full_name:
                return bm
        print('Error: benchmark', name, 'cannot be found')
        quit()

    def add_set(self, set_name, bm_names):
        bms = []
        for name in bm_names:
            bms.append(self.get_bm(name))
        self.__bm_sets[set_name] = bms

    def get_choices(self):
        choices = []
        for bm in self.__bms:
            choices.append(bm.full_name)
            choices.append(bm.abbr_name)
        for bm_set in self.__bm_sets:
            choices.append(bm_set)
        choices.append('all')
        return choices

    def get_selected(self, names):
        if 'all' in names:
            return self.__bms
        else:
            selected = []
            for name in names:
                if name in self.__bm_sets:
                    selected += self.__bm_sets[name]
                else:
                    selected.append(self.get_bm(name))
            return selected


######################################
# Below is project-dependant setting #
######################################

designs = [
    2, 5, 6, 7, 10, 11, 12, 15, 16, 17, 20, 21, 22, 25, 26, 27, 30, 31, 32, 35,
    36, 37, 40, 41, 42, 45, 46, 47, 50, 51, 52, 55, 56, 57, 60, 61, 62, 65, 66,
    67, 70, 71, 72, 75, 76, 77, 80, 81, 82, 85, 86, 87, 90, 91, 92, 95, 96, 97,
    100, 101, 102, 105, 106, 107, 110, 111, 112, 115, 116, 117, 120, 121, 122, 
    125, 126, 127, 130, 131, 132, 135, 136, 137, 140, 141, 142, 145, 147, 150,
    151, 152, 155, 156, 160, 161, 162, 165, 166, 167, 170, 171, 172, 175, 176,
    180, 181, 182, 185, 186, 187, 190, 191, 192, 195, 196, 197, 200, 201, 202,
    205, 206, 207, 210, 211, 212, 215, 216, 217, 220, 221, 222, 225, 226, 227,
    230, 231, 232, 235, 236, 237, 240, 438
]
# all benchmarks
all_benchmarks = Benchmarks()
all_benchmarks.add('Design_{}', 'd{}', designs)

all_benchmarks.add_set('all', ['d{}'.format(d) for d in designs])
