
#####################################################################
# This file is used to generate a setenv.csh script in the installation
# directory that can be used to setup one's environment to use the
# programs and libraries installed there. This takes the place of the
# mk_setenv.csh script in the Hall-D scripts directory.
#
# This should get called anytime the user specifies the "install" target
# on the command line. e.g.:
#
#  scons -u install
#
# It is called from the bottom of the top-level SConstruct file
#
# Nov. 5, 2013  DL
#####################################################################

import os, sys
import subprocess
import datetime
from stat import *


##################################
# mk_setenv_csh
##################################
def mk_setenv_csh(env):
	ofdir = '%s' % env.Dir(env['INSTALLDIR'])
	ofname = '%s/setenv.csh' % ofdir
	print 'sbms : Making setenv.csh in %s' % ofdir
	
	halld_home = '%s' % env.Dir("#/..").srcnode().abspath

	str = ''

	# Header
	str += '#!/bin/tcsh\n'
	str += '#\n'
	str += '# This file was generated by the SBMS system (see SBMS/sbms_setenv.py)\n'
	str += '#\n'
	str += '# Generation date: %s\n' % datetime.datetime.now().strftime("%I:%M%p on %B %d, %Y")
	str += '#\n'
	str += '#       User: %s\n' % os.getenv('USER', 'Unknown')
	str += '#       Host: %s\n' % os.getenv('HOST', 'Unknown')
	str += '#   platform: %s' % subprocess.Popen(["uname", "-a"], stdout=subprocess.PIPE).communicate()[0]
	str += '# BMS_OSNAME: %s\n' % env['OSNAME']
	str += '\n'
	str += '\n'

	# (DY)LD_LIBRARY_PATH
	# On BSD systems, DYLD_LIBRARY_PATH is used while on most others
	# (including Linux) LD_LIBRARY_PATH is used. Here, we set the LDLPV
	# variable to name of the variable to set. We determine this by whether
	# or not DYLD_LIBRARY_PATH is set. At the same time, we make sure the
	# variable is at least defined (empty if needed) so the rest of the
	# generated script can just add to it without checking for existence.
	LDLPV='LD_LIBRARY_PATH'
	if 'Darwin' in env['OSNAME'] : LDLPV='DYLD_LIBRARY_PATH'

	str += '# Make sure %s is set\n' % LDLPV
	str += 'if ( ! $?%s ) then\n' % LDLPV
	str += '   setenv %s\n' % LDLPV
	str += 'endif\n'
	str += '\n'

	str += '# Make sure PYTHONPATH is set\n'
	str += 'if ( ! $?PYTHONPATH ) then\n'
	str += '   setenv PYTHONPATH\n'
	str += 'endif\n'
	str += '\n'

	# CLANG-LLVM C++ compiler
	# If the CLANGROOT environment variable is set, check if there is
	# a setenv.csh script in there that we can source.
	clangroot = os.getenv('CLANGROOT')
	if clangroot != None:
		if os.path.isfile(clangroot):
			str += '# CLANG-LLVM C++ compiler\n'
			str += 'if ( -e %s/setenv.csh ) then\n' % clangroot
			str += '  source %s/setenv.csh\n' % clangroot
			str += 'endif\n'
			str += '\n'
	
	# HDDS
	str += '# HDDS\n'
	str += 'setenv HDDS_HOME %s\n' % os.getenv('HDDS_HOME', '$HOME/hdds')
	str += '\n'

	# JANA
	str += '# JANA\n'
	str += 'setenv JANA_HOME %s\n' % os.getenv('JANA_HOME', '$HOME/jana')
	str += 'setenv JANA_CALIB_URL %s\n' % os.getenv('JANA_CALIB_URL', '$HOME/halld/calib')
	str += 'setenv JANA_GEOMETRY_URL xmlfile://${HDDS_HOME}/main_HDDS.xml\n'
	str += 'setenv JANA_PLUGIN_PATH ${JANA_HOME}/plugins:${JANA_HOME}/lib\n'
	str += 'setenv PATH ${JANA_HOME}/bin:${PATH}\n'
	str += '\n'

	# HALLD
	str += '# HALLD\n'
	str += 'setenv HALLD_HOME %s\n' % halld_home
	str += 'setenv BMS_OSNAME %s\n' % env['OSNAME']
	str += 'setenv PATH ${HALLD_HOME}/${BMS_OSNAME}/bin:${PATH}\n'
	str += 'setenv JANA_PLUGIN_PATH ${HALLD_HOME}/${BMS_OSNAME}/plugins:${JANA_PLUGIN_PATH}\n'
	# python support
	str += 'setenv %s ${HALLD_HOME}/${BMS_OSNAME}/lib:${%s}\n' %(LDLPV, LDLPV)
	str += 'setenv PYTHONPATH ${HALLD_HOME}/${BMS_OSNAME}/lib/python:${PYTHONPATH}\n'
	str += '\n'

	# CCDB
	ccdb_home = os.getenv('CCDB_HOME')
	if ccdb_home != None:
		str += '# CCDB\n'
		str += 'setenv CCDB_HOME %s\n' % os.getenv('CCDB_HOME', '$HOME/ccdb')
		str += 'if ( -e $CCDB_HOME/environment.csh ) then\n'
		str += '  source $CCDB_HOME/environment.csh\n'
		str += 'endif\n'
		str += 'setenv CCDB_CONNECTION ${JANA_CALIB_URL}\n'
		str += '\n'

	# RCDB
	rcdb_home = os.getenv('RCDB_HOME')
	rcdb_conn = os.getenv('RCDB_CONNECTION', 'mysql://rcdb@hallddb.jlab.org/rcdb')
	if rcdb_home != None:
		str += '# RCDB\n'
		str += 'setenv RCDB_HOME %s\n' % rcdb_home
		str += 'setenv RCDB_CONNECTION %s\n' % rcdb_conn
		str += 'setenv %s ${RCDB_HOME}/cpp/lib:${%s}\n' % (LDLPV, LDLPV)
		str += 'setenv PYTHONPATH ${RCDB_HOME}/python:${PYTHONPATH}\n'
		str += 'setenv PATH ${RCDB_HOME}/bin:${RCDB_HOME}/cpp/bin:${PATH}\n'
		str += '\n'

	# ROOT
	str += '# ROOT\n'
	str += 'setenv ROOTSYS %s\n' % os.getenv('ROOTSYS', '$HOME/root')
	str += 'setenv PATH ${ROOTSYS}/bin:${PATH}\n'
	str += 'setenv %s ${ROOTSYS}/lib:${%s}\n' % (LDLPV, LDLPV)
	str += '\n'

	# CERNLIB
	cern = os.getenv('CERN')
	if cern != None:
		str += '# CERNLIB\n'
		str += 'setenv CERN %s\n' % cern
		str += 'setenv CERN_LEVEL %s\n' % os.getenv('CERN_LEVEL', '2006')
		str += 'setenv PATH ${CERN}/${CERN_LEVEL}/bin:${PATH}\n'
		str += 'setenv %s ${CERN}/${CERN_LEVEL}/lib:${%s}\n' % (LDLPV, LDLPV)
		str += '\n'

	# Java
	javaroot = os.getenv('JAVAROOT')
	if javaroot != None:
		str += '# Java\n'
		str += 'setenv JAVAROOT %s\n' % javaroot
		str += '\n'

	# Xerces
	str += '# Xerces\n'
	str += 'setenv XERCESCROOT %s\n' % os.getenv('XERCESCROOT', '$HOME/xerces')
	str += 'setenv PATH ${XERCESCROOT}/bin:${PATH}\n'
	str += 'setenv %s ${XERCESCROOT}/lib:${%s}\n' % (LDLPV, LDLPV)
	str += '\n'
	
	# EVIO
	evioroot = os.getenv('EVIOROOT')
        if evioroot != None:
		str += '# EVIO\n'
		str += 'setenv EVIOROOT %s\n' % evioroot
		str += 'setenv %s ${EVIOROOT}/lib:${%s}\n' % (LDLPV, LDLPV)
	
	# ET
	etroot = os.getenv('ETROOT')
        if etroot != None:
		str += '# ET\n'
                str += 'setenv ETROOT %s\n' % etroot
                str += 'setenv %s ${ETROOT}/lib:${%s}\n' % (LDLPV, LDLPV)

	# Make sure output directory exists
	try:
		os.mkdir(ofdir)
	except OSError:
		pass

	# Write to file
	f = open(ofname, 'w')
	f.write(str)
	f.close()
	os.chmod(ofname, S_IRWXU + S_IRGRP + S_IXGRP + S_IROTH + S_IXOTH)



##################################
# mk_setenv_bash
##################################
def mk_setenv_bash(env):
	ofdir = '%s' % env.Dir(env['INSTALLDIR'])
	ofname = '%s/setenv.sh' % ofdir
	print 'sbms : Making setenv.sh in %s' % ofdir
	
	halld_home = '%s' % env.Dir("#/..").srcnode().abspath

	str = ''

	# Header
	str += '#!/bin/bash\n'
	str += '#\n'
	str += '# This file was generated by the SBMS system (see SBMS/sbms_setenv.py)\n'
	str += '#\n'
	str += '# Generation date: %s\n' % datetime.datetime.now().strftime("%I:%M%p on %B %d, %Y")
	str += '#\n'
	str += '#       User: %s\n' % os.getenv('USER', 'Unknown')
	str += '#       Host: %s\n' % os.getenv('HOST', 'Unknown')
	str += '#   platform: %s' % subprocess.Popen(["uname", "-a"], stdout=subprocess.PIPE).communicate()[0]
	str += '# BMS_OSNAME: %s\n' % env['OSNAME']
	str += '\n'
	str += '\n'

	# (DY)LD_LIBRARY_PATH
	# On BSD systems, DYLD_LIBRARY_PATH is used while on most others
	# (including Linux) LD_LIBRARY_PATH is used. Here, we set the LDLPV
	# variable to name of the variable to set. We determine this by whether
	# or not DYLD_LIBRARY_PATH is set. At the same time, we make sure the
	# variable is at least defined (empty if needed) so the rest of the
	# generated script can just add to it without checking for existence.
	LDLPV='LD_LIBRARY_PATH'
	if 'Darwin' in env['OSNAME'] : LDLPV='DYLD_LIBRARY_PATH'

	str += '# Make sure %s is set\n' % LDLPV
	str += 'if [ -z "$%s" ]; then\n' % LDLPV
	str += '   export %s=""\n' % LDLPV
	str += 'fi\n'
	str += '\n'

	str += '# Make sure PYTHONPATH is set\n'
	str += 'if [ -z "$%s" ]; then\n'
	str += '   export PYTHONPATH=""\n'
	str += 'fi\n'
	str += '\n'

	# CLANG-LLVM C++ compiler
	# If the CLANGROOT environment variable is set, check if there is
	# a setenv.csh script in there that we can source.
	clangroot = os.getenv('CLANGROOT')
	if clangroot != None:
		if os.path.isfile(clangroot):
			str += '# CLANG-LLVM C++ compiler\n'
			str += 'if [ -e %s/setenv.csh ]; then\n' % clangroot
			str += '  . %s/setenv.sh\n' % clangroot
			str += 'fi\n'
			str += '\n'
	
	# HDDS
	str += '# HDDS\n'
	str += 'export HDDS_HOME=%s\n' % os.getenv('HDDS_HOME', '$HOME/hdds')
	str += '\n'

	# JANA
	str += '# JANA\n'
	str += 'export JANA_HOME=%s\n' % os.getenv('JANA_HOME', '$HOME/jana')
	str += 'export JANA_CALIB_URL=%s\n' % os.getenv('JANA_CALIB_URL', '$HOME/halld/calib')
	str += '[ ! -z "$HDDS_HOME" ] && export JANA_GEOMETRY_URL=xmlfile://${HDDS_HOME}/main_HDDS.xml\n'
	str += 'export JANA_PLUGIN_PATH=${JANA_HOME}/plugins:${JANA_HOME}/lib\n'
	str += 'export PATH=${JANA_HOME}/bin:${PATH}\n'
	str += '\n'

	# HALLD
	str += '# HALLD\n'
	str += 'export HALLD_HOME=%s\n' % halld_home
	str += 'export BMS_OSNAME=%s\n' % env['OSNAME']
	str += 'export PATH=${HALLD_HOME}/${BMS_OSNAME}/bin:${PATH}\n'
	str += 'export JANA_PLUGIN_PATH=${HALLD_HOME}/${BMS_OSNAME}/plugins:${JANA_PLUGIN_PATH}\n'
	# python support
	str += 'export %s=${HALLD_HOME}/${BMS_OSNAME}/lib:${%s}\n' %(LDLPV, LDLPV)
	str += 'export PYTHONPATH=${HALLD_HOME}/${BMS_OSNAME}/lib/python:${PYTHONPATH}\n'
	str += '\n'

	# CCDB
	ccdb_home = os.getenv('CCDB_HOME')
	if ccdb_home != None:
		str += '# CCDB\n'
		str += 'export CCDB_HOME=%s\n' % os.getenv('CCDB_HOME', '$HOME/ccdb')
		str += 'if [ -e $CCDB_HOME/environment.bash ]; then\n'
		str += '  . $CCDB_HOME/environment.bash\n'
		str += 'fi\n'
		str += 'export CCDB_CONNECTION=${JANA_CALIB_URL}\n'
		str += '\n'

	# RCDB
	rcdb_home = os.getenv('RCDB_HOME')
	rcdb_conn = os.getenv('RCDB_CONNECTION', 'mysql://rcdb@hallddb.jlab.org/rcdb')
	if rcdb_home != None:
		str += '# RCDB\n'
		str += 'export RCDB_HOME=%s\n' % rcdb_home
		str += 'if [ -e $RCDB_HOME/environment.bash ]; then\n'
		str += '  . $RCDB_HOME/environment.bash\n'
		str += 'fi\n'
		str += 'export RCDB_CONNECTION=%s\n' % rcdb_conn
		str += '\n'

	# ROOT
	str += '# ROOT\n'
	str += 'export ROOTSYS=%s\n' % os.getenv('ROOTSYS', '$HOME/root')
	str += 'export PATH=${ROOTSYS}/bin:${PATH}\n'
	str += 'export %s=${ROOTSYS}/lib:${%s}\n' % (LDLPV, LDLPV)
	str += '\n'

	# CERNLIB
	cern = os.getenv('CERN')
	if cern != None:
		str += '# CERNLIB\n'
		str += 'export CERN=%s\n' % cern
		str += 'export CERN_LEVEL=%s\n' % os.getenv('CERN_LEVEL', '2006')
		str += 'export PATH=${CERN}/${CERN_LEVEL}/bin:${PATH}\n'
		str += 'export %s=${CERN}/${CERN_LEVEL}/lib:${%s}\n' % (LDLPV, LDLPV)
		str += '\n'

	# Java
	javaroot = os.getenv('JAVAROOT')
	if javaroot != None:
		str += '# Java\n'
		str += 'export JAVAROOT=%s\n' % javaroot
		str += '\n'

	# Xerces
	str += '# Xerces\n'
	str += 'export XERCESCROOT=%s\n' % os.getenv('XERCESCROOT', '$HOME/xerces')
	str += 'export PATH=${XERCESCROOT}/bin:${PATH}\n'
	str += 'export %s=${XERCESCROOT}/lib:${%s}\n' % (LDLPV, LDLPV)
	str += '\n'

	# EVIO
        evioroot = os.getenv('EVIOROOT')
        if evioroot != None:
		str += '# EVIO\n'
        	str += 'export EVIOROOT=%s\n' % evioroot
        	str += 'export %s=${EVIOROOT}/lib:${%s}\n' % (LDLPV, LDLPV)

        # ET
        etroot = os.getenv('ETROOT')
        if etroot != None:
		str += '# ET\n'
        	str += 'export ETROOT=%s\n' % etroot
        	str += 'export %s=${ETROOT}/lib:${%s}\n' % (LDLPV, LDLPV)

	# Make sure output directory exists
	try:
		os.mkdir(ofdir)
	except OSError:
		pass

	# Write to file
	f = open(ofname, 'w')
	f.write(str)
	f.close()
	os.chmod(ofname, S_IRWXU + S_IRGRP + S_IXGRP + S_IROTH + S_IXOTH)


