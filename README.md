# PKU_ICS-fall2022

This repository provides some course materials for Introduction to Computer Systems (ICS) in PKU (fall, 2022), including slides, labs and exams. The contents of each folder are described as follow.

- slides: All the slides used in large class teaching (fall, 2022) are included. The slides are almost same each year. This can provide convenience for student who wants to read through before class or take notes on the slides during class.
- labs: Six labs done in PKU's ICS (fall, 2022) course, including datalab, archlab, cachelab, shelllab (or called tshlab), malloclab and proxylab. You can find writeup, handout and my solutions in the folder. Since bomblab and attacklab are online projects and you should finish them on class machines instead of on your own PC, this folder didn't provide the handouts and solutions for them. Another version of bomblab and attacklab (from CMU) will provide in "labs from cmu". An annoying thing is that these labs will be modified almost each year, depends on the TAs that year. This year, we met a new trace in shelllab and a confusing https trace in proxylab. Besides, we met a more difficult version of malloclab. That's what PKU like, isn't it? :( Besides, here are some things you should know:
  - In bomblab, you may get a score deduction once you make the bomb explode. A tricky way is to edit the bomb so that it can run on your own PC and won't inform Autolab even you make the bomb explode. After you work out all the phases, run the origin bomb and type the correct answer to inform Autolab that you're finished. See https://infedg.xyz/index.php/archives/35 to get a hint of how to run your bomb on your own PC. However, the TAs will update bomblab almost every year, trying to fail these attempts. As a result, if you only follow the link I showed above, it may not work. At least, in the ICS (fall, 2022), you need to modify something more. The basic idea of these attempts is to avoid the code for checking running machine and sending message to Autolab. You can know what a function does just from its name.
  - In malloclab, your "throughput" score depends on the performance of the machine. The performance of Autolab is as same as that of class machines, which is always a lot weaker than your own PC. **So, if you are taking ICS in PKU, I strongly suggest you finish malloclab on class machines.**
  - In proxylab, sometimes you may met unstable scores in "realpages" if you are working on class machines. Besides, some failure in "realpages" may be caused by unstable network connections. **So, if you believe your code is correct, try to hand it in Autolab, the score might be different.**
  - **My solutions are only for reference. I am not responsible for any consequence caused by improper use of my solutions, including but not limited to copy my solution and hand in.**
- labs from cmu: I provide a self-learn writeup and handout for bomblab and attacklab, which is from the website of CS:APP ( http://csapp.cs.cmu.edu/ ). You can finish them on your own PC. I didn't write down the solutions for this version of bomblab and attacklab. Instead, you can refer to the following links:
  - bomblab: https://zhuanlan.zhihu.com/p/472178808
  - attacklab: https://zhuanlan.zhihu.com/p/476396465
- midterm exam: The problems and solutions for midterm exams of ICS in PKU, from year 2012 to year 2022. The problems and solutions for midterm exam of ICS (fall, 2022) were not found. **You should keep in mind that there may be many problems which are confusing, disgusting, meaningless, or even have serious errors. As a result, when you are working on these problems, you should always distinguish them carefully.**
- final exam: The problems and solutions for midterm exams of ICS in PKU, from year 2012 to year 2022. The problems and solutions for final exam of ICS (fall, 2022 and fall, 2012), and the solutions for final exam of ICS (fall, 2017) were not found. **You should keep in mind that there may be many problems which are confusing, disgusting, meaningless, or even have serious errors. As a result, when you are working on these problems, you should always distinguish them carefully.**

**This repository is only for reference. I am not responsible for any consequence caused by improper use of this repository, including but not limited to copy my solution and hand in.** I sincerely hope that this repository can help you pull through when taking course Introduction to Computer Systems in PKU. Besides, I hope the quality of ICS in PKU can be improved in a near future.

Last updated: Jan 30th, 2023

------

这是北京大学计算机导论课程（2022年秋）的相关资料，包括课件、lab和往年考试题。各文件夹的内容如下：

- slides：2022年秋季大班课的所有授课课件。每一年的课件几乎一致，因此你可以在课前提前预习课件并在上课时在此课件上做笔记。
- labs：2022年秋季ICS课程的六个lab，包括datalab, archlab, cachelab, shelllab(也称tshlab), malloclab和proxylab。文件夹中包含了这六个lab的writeup, handout和我的解答。因为bomblab和attacklab需要联网在class machines上完成，故此文件夹中并未提供这两个lab的handout和我的解答。另一版本的bomblab和attacklab（来自CMU）可以在labs from cmu文件夹中找到。令人不悦的是，这些lab几乎每年均会被当年的助教所修改。今年，在shelllab中新增了一个trace，在proxylab中新增了一个令人迷惑的https trace。除此以外，今年的malloclab难度也比去年更大。这果然是PKU的风格。:( 此外，以下还有几点注意事项：
  - 在bomblab中，当你引爆炸弹时将会被扣分。一个技巧是所谓的“本地化”，即让bomblab在本地运行且不通知Autolab炸弹爆炸的情况。当你解出所有phase之后，运行原有炸弹并输入所有的正确答案来完成bomblab。关于如何完成bomblab的“本地化”，可以参考这个链接： https://infedg.xyz/index.php/archives/35 。然而，助教基本每年会更新bomblab，来使得这些尝试失效。因此，如果你仅按照上述链接中提到的进行操作，可能无法奏效。至少，在2022年秋季的ICS课程中，需要做更多的修改。“本地化”的基本想法是绕过检查运行机器与联系Autolab的代码。同时，你可以根据函数的名称推断其功能。
  - 在malloclab中，throughput的成绩与机器性能有关。Autolab的性能与class machines的性能相同，通常远弱于你的电脑。**因此，如果你在上北京大学的ICS课程，我强烈建议你在class machines上完成malloclab。**
  - 在proxylab中，realpages的成绩在class machines上可能会出现波动。此外，网络连接不稳定也会导致realpages中的一些trace无法通过。**因此，如果你相信自己的代码没有问题，可以尝试提交到Autolab上，成绩也许会不同。**
  - **我的解答仅供参考。我不对任何对我的解答的不当使用（包括但不限于抄袭代码并提交等行为）所造成的后果负任何责任。**
- labs from cmu：一个bomblab和attacklab的自学版本，包含writeup和handout，来自于CS:APP的网站 ( http://csapp.cs.cmu.edu/ )。自学版本可以在个人电脑上完成。我没有完成这一版本的解答，你可以参考以下链接：
  - bomblab: https://zhuanlan.zhihu.com/p/472178808
  - attacklab: https://zhuanlan.zhihu.com/p/476396465
- midterm exam：北京大学计算机系统导论课程2012年至2022年期中试题的题目与解答，其中2022年期中试题与解答缺失。**请始终牢记：往年试题中，可能有很多令人迷惑、令人恶心、毫无意义的题目，甚至题目本身可能就有严重错误。因此，在做这些往年试题时，请仔细甄别。**
- final exam：北京大学计算机系统导论课程2012年至2022年期末试题的题目与解答，其中2022, 2012年期末试题与解答以及2017年期末解答缺失。**请始终牢记：往年试题中，可能有很多令人迷惑、令人恶心、毫无意义的题目，甚至题目本身可能就有严重错误。因此，在做这些往年试题时，请仔细甄别。**

**这份资料仅供参考。我不对任何对这份资料的不当使用（包括但不限于抄袭代码并提交等行为）所造成的后果负任何责任。**希望这份资料能够帮助你“渡劫”成功北京大学的ICS课程。也真诚希望北京大学的ICS教学质量能够在将来得到提升。

最后更新于2023/1/30
