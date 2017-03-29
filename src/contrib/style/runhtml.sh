#!/bin/bash

OUTFILE=src/contrib/style/html/index.html

echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" > $OUTFILE
echo "<html>" >> $OUTFILE
echo "<head><title>PETSc Style Violation Summary</title>" >> $OUTFILE
echo "<link href=\"style.css\" rel=\"stylesheet\" type=\"text/css\"></head>" >> $OUTFILE
echo "<body><div class=\"main\">" >> $OUTFILE
echo "<h1>PETSc Style Violation Summary</h1>" >> $OUTFILE
echo "<p>This page is a summary of the output generated by the style checker scripts located in \$PETSC_DIR/src/contrib/style. The purpose of these scripts is to promote closer adherence to the PETSc Style Guide located in the <a href=\"http://www.mcs.anl.gov/petsc/developers/developers.pdf\" title=\"PETSc Developer Manual\">PETSc Developer Manual</a>. Keep in mind that the scripts may not be perfect and may produce a number of <a href=\"http://en.wikipedia.org/wiki/Type_I_and_type_II_errors\">type I and type II errors</a>.</p>" >> $OUTFILE

echo "<br /><center><table>" >> $OUTFILE
echo "<tr><th>Rule</th><th>Lines</th></tr>" >> $OUTFILE

for f in `ls src/contrib/style/checks/*.sh`
do
  echo "<tr><td class=\"desc\">" >> $OUTFILE
  grep "# Rule" $f >> $OUTFILE

  # Run script and push results to textfile:
  retstring=`$f`
  retfile=${f%.sh}.txt
  retfile=${retfile/style\/checks/style\/html}
  echo "$retstring" > $retfile
  shortretfile=`basename $retfile`
  echo $shortretfile

  # Write number of lines matched to HTML file and link to text file:
  retval=`$f | wc -l`
  if [ "$retval" -lt "10" ]
  then
    echo "</td><td class=\"green\"><a href=\"$shortretfile\">$retval</a></td></tr>" >> $OUTFILE
  elif [ "$retval" -lt "500" ]
  then
    echo "</td><td class=\"yellow\"><a href=\"$shortretfile\">$retval</a></td></tr>" >> $OUTFILE
  else
    echo "</td><td class=\"red\"><a href=\"$shortretfile\">$retval</a></td></tr>" >> $OUTFILE
  fi
done

echo "</table></center>" >> $OUTFILE
echo "</div></body></html>" >> $OUTFILE
