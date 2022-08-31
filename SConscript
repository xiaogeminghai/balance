
import os
from building import *

cwd = GetCurrentDir()

src = Split('''
balance/balance.c
''')
path = [cwd]

group = DefineGroup('balance', src, depend = ['PKG_USING_BALANCE'], CPPPATH = path)

Return('group')
