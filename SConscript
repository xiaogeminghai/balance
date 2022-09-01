
import os
from building import *

cwd = GetCurrentDir()

src = Split('''
balance/balance.c
''')
path = [cwd+'/balance']

group = DefineGroup('balance', src, depend = ['PKG_USING_BALANCE'], CPPPATH = path)

Return('group')
