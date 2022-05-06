import java.io.IOException;
import java.util.StringTokenizer;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;

public class SleepFreq {

    public static class TokenizerMapper extends Mapper < Object, Text, Text, IntWritable > {
        private final static IntWritable one = new IntWritable(1);
        private Text word = new Text();
        private String hour = "";

        public void map(Object key, Text value, Context context) throws IOException, InterruptedException {
            String tweet = value.toString();

            String[] components = tweet.split("\n");

            String withoutT = components[0].substring(1);
            String Tstripped = withoutT.trim();
            String[] parts = Tstripped.split(" ");
            String time = parts[1];
            hour = time.split(":")[0];   

            String withoutW = components[2].substring(1);
            String tweetContent = withoutW.trim();
            String[] tweetWords = tweetContent.split(" ");

            for(int i = 0; i < tweetWords.length; i++) { 
                if(tweetWords[i].equals("sleep")) {
                    word.set(hour);
                    context.write(word, one);      
                    return;
                }
            }
        }
    }

    public static class IntSumReducer extends Reducer < Text, IntWritable, Text, IntWritable > {
        private IntWritable result = new IntWritable();

        public void reduce(Text key, Iterable < IntWritable > values, Context context) throws IOException, InterruptedException {
            int sum = 0;
            for (IntWritable val: values) {
                sum += val.get();
            }
            result.set(sum);
            context.write(key, result);
        }
    }

    public static void main(String[] args) throws Exception {
        Configuration conf = new Configuration();
        conf.set("textinputformat.record.delimiter", "\n\n");
        Job job = Job.getInstance(conf, "word count");
        job.setJarByClass(SleepFreq.class);
        job.setMapperClass(TokenizerMapper.class);
        job.setCombinerClass(IntSumReducer.class);
        job.setReducerClass(IntSumReducer.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(IntWritable.class);
        FileInputFormat.addInputPath(job, new Path(args[0]));
        FileOutputFormat.setOutputPath(job, new Path(args[1]));
        System.exit(job.waitForCompletion(true) ? 0 : 1);
    }
}