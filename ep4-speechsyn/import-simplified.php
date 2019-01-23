

    <?php
    $load = Array(['a',   [10,223]     , 'choose_randomly', 1.0],
                  ['e',   [33,245]     , 'choose_randomly', 1.0],
                  ['i',   [67,275]     , 'choose_randomly', 1.0],
                  ['o',   [87,302]     , 'choose_randomly', 1.0],
                  ['u',   [118,322]    , 'choose_randomly', 1.0],
                  ['y',   [148,346]    , 'choose_randomly', 1.0],
                  ['ä',   [170,373]    , 'choose_randomly', 1.0],
                  ['ö',   [201,398]    , 'choose_randomly', 1.0],
                  ['l',   [19]         , 'choose_randomly', 1.0],
                  ['v',   [47,48,49,50,51], 'play_in_sequence',1.0],
                  ['j',   [77,78,79]   , 'play_in_sequence',1.0],
                  ['m',   [105,108]    , 'choose_randomly', 1.0],
                  ['n',   [129]        , 'choose_randomly', 1.0],
                  ['ŋ',   [158]        , 'choose_randomly', 1.0],
                  ['r',   [186,189]    , 'trill',           0.7],
                  ['h',   [229,233,234], 'choose_randomly', 0.8],
                  ['s',   [259]        , 'choose_randomly', 0.6],
                  ['M',   [108,109,110], 'play_in_sequence', 1.0],
                  ['N',   [136]        , 'play_in_sequence', 1.0],
                  ['Ŋ',   [162,163,164,165], 'play_in_sequence', 1.0],
                  ['k',   [291,292,293], 'play_in_sequence',0.7],
                  ['p',   [315,316]    , 'play_in_sequence',0.9],
                  ['t',   [341,342,343], 'play_in_sequence',0.7],
                  ['d',   [366,367,368], 'play_in_sequence',1.0],
                  ['q',   [214,291]    , 'choose_randomly', 1.0],
                  ['-',   [385,386]    , 'choose_randomly', 1.0]
            );

    /* Yes, I use PHP. Because a programming language, that you know is much more efficient than one that you don't know. */
    $frames = Array();
    $frame  = Array();
    foreach(file('ahesiko44100-48b.LPC') as $line)
    {
      if(sscanf($line, "            a [%d] = %f", $index, $coeff) == 2) 
        $frame[] = $coeff;
      elseif(sscanf($line, "        gain = %f", $gain) == 1)
        { $frames[] = Array('f'=>$frame, 'g'=>$gain); $frame = Array(); }
    }
    foreach($load as $d)
    {
      printf("  {U'%s', { record_modes::%s, %.1f, {\n", $d[0], $d[2], $d[3]);
      foreach($d[1] as $c=>$index)
      {
        if($c) print ",\n";
        $frame = $frames[$index];

        printf("    {%6.5f,{", $frame['g']);
        $sep = '';
        $n=0;
        foreach($frame['f'] as $f)
        {
          print $sep; $sep = ',';

          printf('%7.6f', $f);
          if(++$n == 24) { $n=0; $sep = ",\n              "; }
        }
        printf(" }}");
      }
      if(!empty($d[1])) print "\n";
      printf("  } } },\n");
    }
