H=$(git rev-parse HEAD)

HF="static String GIT_HASH=\"${H}\";"

echo $HF >gitVersion.h
