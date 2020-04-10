project('cpiofs', 
	'c', 
	default_options: ['c_std=c11'],
	version: '0.1',
    license: 'BSD',
)

inc = include_directories('.')

easyzmq = library('cpiofs', 
	['cpiofs.c'], 
	include_directories : inc)

executable('test1', 
	['test1.c'], 
	include_directories : inc,
	link_with : [easyzmq])

# to create test archive 
# ./test/build.sh
