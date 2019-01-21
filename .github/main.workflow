workflow "New workflow" {
  on = "push"
  resolves = ["Compile"]
}

action "Compile" {
  uses = "actions/aws/cli@8d318706868c00929f57a0b618504dcdda52b0c9"
  runs = "make clean & make"
}
