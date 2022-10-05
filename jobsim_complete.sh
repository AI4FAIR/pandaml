#!/bin/bash

# Lustre
LUSTRE_HOME="/lustre/panda/"$USER
nyx=$LUSTRE_HOME"/pandaml"
tmpdir="/tmp/"$USER
_target=$nyx"/data"


# PandaRoot
. $LUSTRE_HOME"/DEBIAN/v13.0.0-install/bin/config.sh" -p


# Default Inputs
nevt=1000
prefix=mumu
gen=DBoxGEN                           # SBoxGEN, DBoxGEN or .DEC
pBeam=1.642                           # llbar: 1.642, xibarxi1820: 4.6 GeV/c
seed=$RANDOM
run=$SLURM_ARRAY_TASK_ID

# User Inputs
if test "$1" != ""; then
  nevt=$1
fi

if test "$2" != ""; then
  prefix=$2
fi

if test "$3" != ""; then
  gen=$3
fi


# IF ARRAY_TASK Used
if test "$SLURM_ARRAY_TASK_ID" == ""; then
    tmpdir="/tmp/"$USER
    outprefix=$tmpdir"/"$prefix
    run=1
    seed=$run
else
    tmpdir="/tmp/"$USER"_"$SLURM_JOB_ID
    outprefix=$tmpdir"/"$prefix"_"$run
    run=$SLURM_ARRAY_TASK_ID
    seed=$run
fi


# Make sure `$_target` Exists
if [ ! -d $_target ]; then
    mkdir -p $_target;
    echo -e "\nThe data dir. at '$_target' created."
else
    echo -e "\nThe data dir. at '$_target' exists."
fi


# Make sure $tempdir exists
if [ ! -d $tmpdir ]; then
    mkdir -p $tmpdir;
    echo -e "The temporary dir. at '$tmpdir' created."
else
    echo -e "The temporary dir. at '$tmpdir' exists."
fi


#*** Print Flags ***
echo -e "\nLustre Home  : $LUSTRE_HOME"
echo -e "Working Dir. : $nyx"
echo -e "Temp Dir.    : $tmpdir"
echo -e "Target Dir.  : $_target"
echo -e "\nEvents    : $nevt"
echo -e "Prefix    : $outprefix"
echo -e "Decay     : $gen"
echo -e "pBeam     : $pBeam"
echo -e "Seed      : $seed"
echo -e "Run       : $run"


# Terminate Script for Testing.
# exit 0;


#*** Initiate Simulaton ***
echo -e "\nStarted Simulating..."
root -l -b -q $nyx"/"sim_complete.C\($nevt,\"$outprefix\",\"$gen\",$pBeam,$seed\) > $outprefix"_sim.log" 2>&1

echo "Started Digitization..."
root -l -b -q $nyx"/"digi_complete.C\($nevt,\"$outprefix\"\) > $outprefix"_digi.log" 2>&1

echo "Started Ideal Reconstruction..."
root -l -b -q $nyx"/"recoideal_complete.C\($nevt,\"$outprefix\"\) > $outprefix"_reco.log" 2>&1

echo "Started CSV Generator..."
root -l -b -q $nyx"/"data_complete.C\($nevt,\"$outprefix\",\"$tmpdir\",$run\) > $outprefix"_data.log" 2>&1

echo -e "Finished Simulating..."


#*** Storing Files ***
echo -e "\nMoving Files from '$tmpdir' to '$_target'"

cp $outprefix"_par.root" $_target
cp $outprefix"_sim.root" $_target
cp $outprefix"_sim.log" $_target
cp $outprefix"_digi.root" $_target
cp $outprefix"_digi.log" $_target
cp $outprefix"_reco.root" $_target
cp $outprefix"_reco.log" $_target
cp $outprefix"_data.root" $_target
cp $outprefix"_data.log" $_target

cp $tmpdir"/"*.csv $_target


#*** Tidy Up ***
rm -rf $tmpdir

echo -e "The Script has Finished wit SLURM_JOB_ID: $run."
