#!/bin/sh

action=$1

set -e
set -u

# Several branches
# Branch upstream: complete svn repo, including branch directory
# Branch master: upstream without branch information and svn-id's in commits
# Branch v4.9, v5.0,...: Branches from upstream

NGDIR=ngsolve-mirror
TMPTEMPLATE=/tmp/ngs_convert.XXXXXXXX
cleanup()
{
    rm -rf /tmp/ngs_convert.*
}
trap cleanup 0 1 2 3 6
GIT_FILTERBRANCH_DIR=$(mktemp -d ${TMPTEMPLATE})
SED_CLEAN_SCRIPT=$(mktemp ${TMPTEMPLATE})
cat > "${SED_CLEAN_SCRIPT}" << EOF
/\tbranches/d
/\t.*\.vol$/d
/\t.*\.vol.gz$/d
/\t.*quickstart.pdf$/d
EOF


if [ "${action}" = "init" ]; then
    git svn clone -A ngs_authors.txt -T ./ --ignore-paths '^(ngsolve-4.9)' svn://svn.code.sf.net/p/ngsolve/code ${NGDIR}
    git remote add github github_mirror:mliertzer/ngsolve.git
    cd ${NGDIR}
    git checkout -b upstream
    git branch -d master
    cd ..
fi

cd ${NGDIR}
git checkout upstream
git svn rebase

branchversions=$(ls -d branches/ngsolve-* | sed -e 's#branches/ngsolve-##')

for i in $(git branch --list master v* | cut -c3-); do  
    git branch -D $i;
done

git gc

git branch master upstream
git filter-branch --index-filter \
     "git ls-files -s | sed -f ${SED_CLEAN_SCRIPT} | GIT_INDEX_FILE=\${GIT_INDEX_FILE}.new git update-index --index-info &&
      mv \"\${GIT_INDEX_FILE}.new\" \"\${GIT_INDEX_FILE}\"" --msg-filter 'sed -e "/^git-svn-id:/d"' --prune-empty -d ${GIT_FILTERBRANCH_DIR} -f master

for bv in ${branchversions}; do
    bn=v${bv}
    git branch ${bn} upstream
    git filter-branch --index-filter\
    "set -e
     set -u
     TMPFILE=\$(mktemp ${TMPTEMPLATE})
     git ls-files -s > \${TMPFILE};
     if grep -q '	branches/ngsolve-${bv}/' \${TMPFILE}; then
          sed '/\tngsolve\//d
s#\tbranches/ngsolve-${bv}/#\tngsolve/#' \${TMPFILE} | sort -k 3 > \${TMPFILE}.bak
          mv \${TMPFILE}.bak \${TMPFILE}
     fi

     if grep -q '	branches/my_little_ngsolve-${bv}/' \${TMPFILE}; then
          sed '/\tmy_little_ngsolve\//d
s#\tbranches/my_little_ngsolve-${bv}/#\tmy_little_ngsolve/#' \${TMPFILE} | sort -k 3 > \${TMPFILE}.bak
          mv \${TMPFILE}.bak \${TMPFILE}
     fi
     

     sed -f ${SED_CLEAN_SCRIPT} \${TMPFILE} > \${TMPFILE}.bak
     mv \${TMPFILE}.bak \${TMPFILE}

     GIT_INDEX_FILE=\"\${GIT_INDEX_FILE}.new\" git update-index --index-info < \${TMPFILE} &&
     mv \"\${GIT_INDEX_FILE}.new\" \"\${GIT_INDEX_FILE}\"
     rm \${TMPFILE}" --msg-filter 'sed -e "/^git-svn-id:/d"' --prune-empty -d ${GIT_FILTERBRANCH_DIR} -f ${bn}
done

git push github +master:master
for bv in ${branchversions}; do
    bn=v${bv}
    git push github +${bn}:${bn}
done

if ! git checkout mirrorinfo; then
    git checkout --orphan mirrorinfo
    git reset --hard
fi
cp ../${0} convert.sh
cp ../ngs_authors.txt .
cat > README.md << EOF
Mirror of the subversion repository of ngsolve
====================================================

This is an unofficial mirror of the ngsolve subversion repository and
is updated every 24 hours. The original repository can be found at

<pre>
svn://svn.code.sf.net/p/ngsolve/code
</pre>

The trunk of the repository can be found in the master branch. All old
branches 4.9,5.0,... are in respective git branches v4.9,v5.0,... .

Since the git repository is created from the original subversion
repository using git filter-branch, fast-forwarding of the individual
branches might not work all the time. In comparison to the svn repo
mesh files (*.vol and *.vol.gz) and the quickstart.pdf file have been
removed in order to reduce the size of the repository. The files can
be reproduced from the source files which are still included in the
repository.

The shell script convert.sh and the authors file for converting the
subversion repository is located together with this README file in the
mirrorinfo branch.

Since this repository is only a mirror, pull requests will _not_ be
accepted/looked at.  For submitting bug reports/patches, see the 
[original website](http://sourceforge.net/projects/ngsolve)
or contact Joachim Schöberl directly.

EOF

git add convert.sh ngs_authors.txt README.md
if git commit -m "Update mirror info"; then
    git push github mirrorinfo:mirrorinfo
fi

git checkout upstream
cd ..
